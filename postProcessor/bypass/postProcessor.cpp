// postprocessor/bypass/postProcessor.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <zmq.hpp>
#include "ImageStructure.hpp"
#include "utils.h"

int main() {
    std::cout << "=== POSTPROCESSOR (BYPASS MODE) STARTED ===" << std::endl;

#ifdef DEBUG
    std::cout << "DEBUG MODE ENABLED" << std::endl;
#endif

    // 1. Загружаем конфигурацию
    Utils config;
    config.loadConfig();

    int port = std::stoi(config.getConfig("postprocessor.zmq_port")); // 5557
    std::string output_dir = config.getConfig("postprocessor.output_dir");
    std::string prefix = config.getConfig("postprocessor.processed_prefix");

    // Создаём директорию вывода
    std::filesystem::create_directories(output_dir);

    // 2. ZeroMQ: PULL-сокет для приёма от worker'а
    zmq::context_t ctx(1);
    zmq::socket_t socket(ctx, zmq::socket_type::pull);
    std::string endpoint = "tcp://*:" + std::to_string(port);
    socket.bind(endpoint);

    std::cout << "Bound to: " << endpoint << std::endl;
    std::cout << "Output dir: " << output_dir << std::endl;
    std::cout << "Filename prefix: " << prefix << std::endl;

    // 3. Основной цикл приёма
    while (true) {
        zmq::message_t msg;
        auto result = socket.recv(msg, zmq::recv_flags::none);

        if (!result.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Десериализуем
        ImageStructure is;
        if (!is.deserialize(msg)) {
            std::cerr << "Failed to deserialize incoming message" << std::endl;
            continue;
        }

        cv::Mat image = is.getImage();
        uint64_t frame_id = is.getFrameId();

        if (image.empty()) {
            std::cerr << "Received empty image" << std::endl;
            continue;
        }

        // Формируем имя файла: proc_123.bmp
        std::string filename = output_dir + prefix + std::to_string(frame_id) + ".bmp";

        // Сохраняем
        if (cv::imwrite(filename, image)) {
            std::cout << "- [ OK ] Saved: " << filename
                      << " (" << image.cols << "x" << image.rows << ")" << std::endl;
        } else {
            std::cerr << "FAILED to save: " << filename << std::endl;
        }
    }

    return 0;
}