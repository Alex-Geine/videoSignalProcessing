#include <iostream>
#include <yaml-cpp/yaml.h>
#include "utils.h"

// Условная компиляция для ZeroMQ
#ifdef ZMQ_FOUND
#include <zmq.hpp>
#include <zmq_addon.hpp>
#endif

// PImpl структура
struct Utils::Impl {
#ifdef ZMQ_FOUND
    zmq::context_t context;
    zmq::socket_t socket;
    
    Impl() : context(1), socket(context, zmq::socket_type::dealer) {}
#else
    Impl() = default;
#endif
};

Utils::Utils() : pImpl(std::make_unique<Impl>()) {
#ifdef ZMQ_FOUND
    // Настройка ZeroMQ сокета
    pImpl->socket.set(zmq::sockopt::rcvtimeo, 1000);
    pImpl->socket.set(zmq::sockopt::sndtimeo, 1000);
#endif
}

Utils::~Utils() = default;

void Utils::processConfig() {
    std::cout << "Utils REAL implementation with yaml-cpp" << std::endl;
    
#ifdef ZMQ_FOUND
    std::cout << "ZeroMQ support: ENABLED" << std::endl;
#else
    std::cout << "ZeroMQ support: DISABLED" << std::endl;
#endif
    
    try {
        YAML::Node config = YAML::Load("{name: 'video_system', version: '1.0.0', zmq_port: 5555}");
        std::cout << "System: " << config["name"].as<std::string>() << std::endl;
        std::cout << "ZMQ Port: " << config["zmq_port"].as<int>() << std::endl;
        
    } catch (const YAML::Exception& e) {
        std::cout << "YAML parsing error: " << e.what() << std::endl;
    }
}

void Utils::sendMessage(const std::string& message) {
#ifdef ZMQ_FOUND
    try {
        zmq::message_t zmq_msg(message.data(), message.size());
        pImpl->socket.send(zmq_msg, zmq::send_flags::dontwait);
        std::cout << "Message sent: " << message << std::endl;
    } catch (const zmq::error_t& e) {
        std::cout << "ZeroMQ send error: " << e.what() << std::endl;
    }
#else
    std::cout << "ZMQ STUB: Would send message: " << message << std::endl;
#endif
}

std::string Utils::receiveMessage() {
#ifdef ZMQ_FOUND
    try {
        zmq::message_t zmq_msg;
        if (auto result = pImpl->socket.recv(zmq_msg, zmq::recv_flags::dontwait)) {
            return std::string(static_cast<char*>(zmq_msg.data()), zmq_msg.size());
        }
    } catch (const zmq::error_t& e) {
        std::cout << "ZeroMQ receive error: " << e.what() << std::endl;
    }
    return "";
#else
    std::cout << "ZMQ STUB: Simulating message receive" << std::endl;
    return "stub_response";
#endif
}

std::string Utils::getVersion() {
#ifdef ZMQ_FOUND
    return "1.0.0-real-with-zmq";
#else
    return "1.0.0-real-no-zmq";
#endif
}

int main() {
    Utils utils;
    utils.processConfig();
    
    utils.sendMessage("Hello from Utils!");
    auto response = utils.receiveMessage();
    if (!response.empty()) {
        std::cout << "Received: " << response << std::endl;
    }
    
    std::cout << "Utils version: " << utils.getVersion() << std::endl;
    return 0;
}