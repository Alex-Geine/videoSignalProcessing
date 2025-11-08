#include <iostream>
#include <fstream>
#include <vector>
#include <yaml-cpp/yaml.h>

// ZeroMQ
#include <zmq.hpp>
#include <zmq_addon.hpp>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include "utils.h"

// PImpl структура
struct Utils::Impl {
    // ZeroMQ
    std::unique_ptr<zmq::context_t> context;
    std::unique_ptr<zmq::socket_t> socket;
    bool connected;
    bool is_server;
    
    // Конфигурация
    YAML::Node config;
    
    // Изображение
    cv::Mat current_image;
    
    Impl() : connected(false), is_server(false) {
        try {
            context = std::make_unique<zmq::context_t>(1);
        } catch (const zmq::error_t& e) {
            std::cout << "ZeroMQ initialization error: " << e.what() << std::endl;
        }
    }
};

Utils::Utils() : pImpl(std::make_unique<Impl>()) {}

Utils::~Utils() {
    if (pImpl->connected && pImpl->socket) {
        pImpl->socket->close();
    }
    if (pImpl->context) {
        pImpl->context->close();
    }
}

// ============================================================================
// КОНФИГУРАЦИЯ
// ============================================================================

void Utils::loadConfig(const std::string& config_path) {
    try {
        pImpl->config = YAML::LoadFile(config_path);
        std::cout << "Configuration loaded from: " << config_path << std::endl;
    } catch (const YAML::Exception& e) {
        std::cout << "Error loading config file: " << e.what() << std::endl;
    }
}

std::string Utils::getConfig(const std::string& node_path) {
    try {
        YAML::Node node = YAML::Clone(pImpl->config);
        std::vector<std::string> tokens;
        
        // Парсим путь вида "server.ip"
        size_t start = 0, end = 0;
        while ((end = node_path.find('.', start)) != std::string::npos) {
            tokens.push_back(node_path.substr(start, end - start));
            start = end + 1;
        }
        tokens.push_back(node_path.substr(start));
        
        // Проходим по узлам
        for (const auto& token : tokens) {
            node = node[token];
        }
        
        return node.as<std::string>();
    } catch (const YAML::Exception& e) {
        std::cout << "Error getting config '" << node_path << "': " << e.what() << std::endl;
        return "";
    }
}

// ============================================================================
// РАБОТА С ИЗОБРАЖЕНИЯМИ
// ============================================================================

bool Utils::loadImage(const std::string& path) {
    pImpl->current_image = cv::imread(path);
    if (pImpl->current_image.empty()) {
        std::cout << "Cannot load image: " << path << std::endl;
        return false;
    }
    std::cout << "Image loaded: " << path << " (" 
              << pImpl->current_image.cols << "x" << pImpl->current_image.rows << ")" << std::endl;
    return true;
}

bool Utils::saveImage(const std::string& path) {
    if (pImpl->current_image.empty()) {
        std::cout << "No image to save" << std::endl;
        return false;
    }
    
    bool success = cv::imwrite(path, pImpl->current_image);
    if (success) {
        std::cout << "Image saved: " << path << std::endl;
    } else {
        std::cout << "Failed to save image: " << path << std::endl;
    }
    return success;
}

cv::Mat Utils::getCurrentImage() {
    return pImpl->current_image.clone();
}

void Utils::setCurrentImage(const cv::Mat& image) {
    pImpl->current_image = image.clone();
}

// ============================================================================
// СЕТЕВОЕ ВЗАИМОДЕЙСТВИЕ
// ============================================================================

bool Utils::initializeServer(const std::string& ip, int port) {
    try {
        std::string address = "tcp://" + ip + ":" + std::to_string(port);
        pImpl->socket = std::make_unique<zmq::socket_t>(*pImpl->context, ZMQ_REP);
        pImpl->socket->bind(address);
        pImpl->connected = true;
        pImpl->is_server = true;
        
        std::cout << "Server started on: " << address << std::endl;
        return true;
    } catch (const zmq::error_t& e) {
        std::cout << "Server initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool Utils::initializeClient(const std::string& ip, int port) {
    try {
        std::string address = "tcp://" + ip + ":" + std::to_string(port);
        pImpl->socket = std::make_unique<zmq::socket_t>(*pImpl->context, ZMQ_REQ);
        pImpl->socket->connect(address);
        pImpl->connected = true;
        pImpl->is_server = false;
        
        // Таймауты
        pImpl->socket->set(zmq::sockopt::rcvtimeo, 5000);
        pImpl->socket->set(zmq::sockopt::sndtimeo, 5000);
        
        std::cout << "Client connected to: " << address << std::endl;
        return true;
    } catch (const zmq::error_t& e) {
        std::cout << "Client initialization error: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// СЕРИАЛИЗАЦИЯ И ДЕСЕРИАЛИЗАЦИЯ ИЗОБРАЖЕНИЙ
// ============================================================================

std::string Utils::serializeImage(const cv::Mat& image) {
    if (image.empty()) {
        return "";
    }
    
    std::vector<uchar> buffer;
    cv::imencode(".bmp", image, buffer);
    
    return std::string(buffer.begin(), buffer.end());
}

cv::Mat Utils::deserializeImage(const std::string& data) {
    if (data.empty()) {
        return cv::Mat();
    }
    
    std::vector<uchar> buffer(data.begin(), data.end());
    return cv::imdecode(buffer, cv::IMREAD_COLOR);
}

// ============================================================================
// ПЕРЕДАЧА ИЗОБРАЖЕНИЙ
// ============================================================================

bool Utils::sendImage(const cv::Mat& image) {
    if (!pImpl->connected || !pImpl->socket) {
        std::cout << "Not connected" << std::endl;
        return false;
    }
    
    try {
        std::string image_data = serializeImage(image);
        if (image_data.empty()) {
            std::cout << "Failed to serialize image" << std::endl;
            return false;
        }
        
        zmq::message_t message(image_data.size());
        memcpy(message.data(), image_data.data(), image_data.size());
        
        auto result = pImpl->socket->send(message, zmq::send_flags::none);
        if (result.has_value()) {
            std::cout << "Image sent (" << image_data.size() << " bytes)" << std::endl;
            return true;
        } else {
            std::cout << "Failed to send image" << std::endl;
            return false;
        }
    } catch (const zmq::error_t& e) {
        std::cout << "Send image error: " << e.what() << std::endl;
        return false;
    }
}

cv::Mat Utils::receiveImage() {
    if (!pImpl->connected || !pImpl->socket) {
        std::cout << "Not connected" << std::endl;
        return cv::Mat();
    }
    
    try {
        zmq::message_t message;
        auto result = pImpl->socket->recv(message, zmq::recv_flags::none);
        
        if (result.has_value() && message.size() > 0) {
            std::string image_data(static_cast<char*>(message.data()), message.size());
            cv::Mat image = deserializeImage(image_data);
            
            if (!image.empty()) {
                std::cout << "Image received (" << message.size() << " bytes, " 
                          << image.cols << "x" << image.rows << ")" << std::endl;
            } else {
                std::cout << "Failed to deserialize received image" << std::endl;
            }
            
            return image;
        } else {
            std::cout << "No image received" << std::endl;
            return cv::Mat();
        }
    } catch (const zmq::error_t& e) {
        std::cout << "Receive image error: " << e.what() << std::endl;
        return cv::Mat();
    }
}

// ============================================================================
// ПРОСТЫЕ СООБЩЕНИЯ
// ============================================================================

void Utils::sendMessage(const std::string& message) {
    if (!pImpl->connected || !pImpl->socket) {
        std::cout << "Not connected" << std::endl;
        return;
    }
    
    try {
        zmq::message_t msg(message.size());
        memcpy(msg.data(), message.data(), message.size());
        
        auto result = pImpl->socket->send(msg, zmq::send_flags::none);
        if (result.has_value()) {
            std::cout << "Message sent: " << message << std::endl;
        } else {
            std::cout << "Failed to send message: " << message << std::endl;
        }
    } catch (const zmq::error_t& e) {
        std::cout << "Send message error: " << e.what() << std::endl;
    }
}

std::string Utils::receiveMessage() {
    if (!pImpl->connected || !pImpl->socket) {
        std::cout << "Not connected" << std::endl;
        return "";
    }
    
    try {
        zmq::message_t message;
        auto result = pImpl->socket->recv(message, zmq::recv_flags::none);
        
        if (result.has_value() && message.size() > 0) {
            std::string received(static_cast<char*>(message.data()), message.size());
            std::cout << "Message received: " << received << std::endl;
            return received;
        } else {
            std::cout << "No message received" << std::endl;
            return "";
        }
    } catch (const zmq::error_t& e) {
        std::cout << "Receive message error: " << e.what() << std::endl;
        return "";
    }
}

// ============================================================================
// СТАТУС
// ============================================================================

bool Utils::isConnected() {
    return pImpl->connected;
}

std::string Utils::getVersion() {
    return "1.0.0-image-processing-system";
}