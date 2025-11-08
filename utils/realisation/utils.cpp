#include <iostream>
#include <yaml-cpp/yaml.h>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

// ZeroMQ headers - ДОЛЖНЫ быть после определения ZMQ_STATIC в CMake
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include "utils.h"

// PImpl структура
struct Utils::Impl {
    // ZeroMQ
    std::unique_ptr<zmq::context_t> context;
    std::unique_ptr<zmq::socket_t> socket;
    bool connected;
    
    // OpenCV
    cv::VideoCapture camera;
    cv::Mat current_frame;
    cv::Mat processed_frame;
    bool camera_initialized;
    
    Impl() : connected(false), camera_initialized(false) {
        // Инициализация ZeroMQ
        try {
            context = std::make_unique<zmq::context_t>(1);
            socket = std::make_unique<zmq::socket_t>(*context, ZMQ_DEALER);
            
            // Используем современный API
            socket->set(zmq::sockopt::rcvtimeo, 1000);
            socket->set(zmq::sockopt::sndtimeo, 1000);
            socket->set(zmq::sockopt::linger, 0);
            
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
    if (pImpl->camera_initialized) {
        pImpl->camera.release();
    }
}

void Utils::processConfig() {
    std::cout << "Utils REAL implementation with yaml-cpp, ZeroMQ and OpenCV" << std::endl;
    
    try {
        YAML::Node config = YAML::Load("{name: 'video_system', version: '1.0.0', camera_index: 0}");
        std::cout << "System: " << config["name"].as<std::string>() << std::endl;
        
        int camera_index = config["camera_index"].as<int>();
        if (initializeCamera(camera_index)) {
            std::cout << "Camera initialized successfully" << std::endl;
        }
        
    } catch (const YAML::Exception& e) {
        std::cout << "YAML parsing error: " << e.what() << std::endl;
    }
}

bool Utils::initializeCamera(int camera_index) {
    if (!pImpl->camera.open(camera_index)) {
        std::cout << "Cannot open camera with index: " << camera_index << std::endl;
        return false;
    }
    
    pImpl->camera_initialized = true;
    
    // Устанавливаем базовые настройки камеры
    pImpl->camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    pImpl->camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    pImpl->camera.set(cv::CAP_PROP_FPS, 30);
    
    std::cout << "Camera opened: " 
              << pImpl->camera.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << pImpl->camera.get(cv::CAP_PROP_FRAME_HEIGHT) << " @ "
              << pImpl->camera.get(cv::CAP_PROP_FPS) << " FPS" << std::endl;
    
    return true;
}

void Utils::processVideoFrame() {
    if (!pImpl->camera_initialized) {
        std::cout << "Camera not initialized. Call initializeCamera() first." << std::endl;
        return;
    }
    
    // Захватываем кадр
    pImpl->camera >> pImpl->current_frame;
    
    if (pImpl->current_frame.empty()) {
        std::cout << "Failed to capture frame from camera" << std::endl;
        return;
    }
    
    std::cout << "Frame captured: " 
              << pImpl->current_frame.cols << "x" << pImpl->current_frame.rows 
              << " channels: " << pImpl->current_frame.channels() << std::endl;
}

void Utils::applyImageFilter() {
    if (pImpl->current_frame.empty()) {
        std::cout << "No frame to process. Call processVideoFrame() first." << std::endl;
        return;
    }
    
    // Применяем Gaussian blur фильтр
    cv::GaussianBlur(pImpl->current_frame, pImpl->processed_frame, cv::Size(15, 15), 0);
    
    // Конвертируем в grayscale для дополнительной обработки
    if (pImpl->current_frame.channels() == 3) {
        cv::cvtColor(pImpl->processed_frame, pImpl->processed_frame, cv::COLOR_BGR2GRAY);
    }
    
    std::cout << "Image filter applied: Gaussian blur + grayscale" << std::endl;
}

void Utils::displayImage() {
    if (pImpl->processed_frame.empty()) {
        std::cout << "No processed frame to display" << std::endl;
        return;
    }
    
    cv::namedWindow("Processed Frame", cv::WINDOW_NORMAL);
    cv::imshow("Processed Frame", pImpl->processed_frame);
    
    std::cout << "Press any key to close the window..." << std::endl;
    cv::waitKey(0);
    cv::destroyAllWindows();
}

// ZeroMQ методы с исправлениями для новой версии
bool Utils::connect(const std::string& address) {
    if (!pImpl->socket) {
        std::cout << "ZeroMQ socket not initialized" << std::endl;
        return false;
    }
    
    try {
        pImpl->socket->connect(address);
        pImpl->connected = true;
        std::cout << "Connected to: " << address << std::endl;
        return true;
    } catch (const zmq::error_t& e) {
        std::cout << "ZeroMQ connect error: " << e.what() << std::endl;
        return false;
    }
}

bool Utils::bind(const std::string& address) {
    if (!pImpl->socket) {
        std::cout << "ZeroMQ socket not initialized" << std::endl;
        return false;
    }
    
    try {
        pImpl->socket->bind(address);
        pImpl->connected = true;
        std::cout << "Bound to: " << address << std::endl;
        return true;
    } catch (const zmq::error_t& e) {
        std::cout << "ZeroMQ bind error: " << e.what() << std::endl;
        return false;
    }
}

void Utils::sendMessage(const std::string& message) {
    if (!pImpl->connected || !pImpl->socket) {
        std::cout << "Not connected to ZeroMQ. Call connect() or bind() first." << std::endl;
        return;
    }
    
    try {
        zmq::message_t zmq_msg(message.size());
        memcpy(zmq_msg.data(), message.data(), message.size());
        
        auto send_result = pImpl->socket->send(zmq_msg, zmq::send_flags::dontwait);
        if (send_result.has_value()) {
            std::cout << "Message sent: " << message << std::endl;
        } else {
            std::cout << "Message not sent (timeout): " << message << std::endl;
        }
    } catch (const zmq::error_t& e) {
        std::cout << "ZeroMQ send error: " << e.what() << std::endl;
    }
}

std::string Utils::receiveMessage() {
    if (!pImpl->connected || !pImpl->socket) {
        std::cout << "Not connected to ZeroMQ. Call connect() or bind() first." << std::endl;
        return "";
    }
    
    try {
        zmq::message_t zmq_msg;
        auto recv_result = pImpl->socket->recv(zmq_msg, zmq::recv_flags::dontwait);
        
        if (recv_result.has_value() && zmq_msg.size() > 0) {
            std::string message(static_cast<char*>(zmq_msg.data()), zmq_msg.size());
            std::cout << "Message received: " << message << std::endl;
            return message;
        } else {
            std::cout << "No message received (timeout or empty)" << std::endl;
            return "";
        }
    } catch (const zmq::error_t& e) {
        std::cout << "ZeroMQ receive error: " << e.what() << std::endl;
        return "";
    }
}

std::string Utils::getVersion() {
    return "1.0.0-real-with-opencv-zeromq";
}