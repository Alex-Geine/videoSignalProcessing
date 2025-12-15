// server/bypass/server.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <zmq.hpp>
#include "ImageStructure.hpp"
#include "utils.h"

int main() {
    std::cout << "=== SERVER (BYPASS MODE) STARTED ===" << std::endl;

#ifdef DEBUG
    std::cout << "DEBUG MODE ENABLED" << std::endl;
#endif

    // 1. Загружаем конфигурацию
    Utils server;
    server.loadConfig();

    std::string input_image_path = server.getConfig("server.input_image");
    std::string zmq_endpoint = "tcp://*:" + server.getConfig("server.zmq_port"); // например, "5555"

    // 2. Загружаем изображение один раз
    cv::Mat image = cv::imread(input_image_path);
    if (image.empty()) {
        std::cerr << "ERROR: Cannot load input image: " << input_image_path << std::endl;
        return -1;
    }
    std::cout << "Loaded emulation image: " << input_image_path
              << " (" << image.cols << "x" << image.rows << ")" << std::endl;

    // 3. Инициализируем ZeroMQ
    zmq::context_t ctx(1);
    zmq::socket_t socket(ctx, zmq::socket_type::push);
    
    try {
        socket.set(zmq::sockopt::sndhwm, 100);
        socket.set(zmq::sockopt::linger, 0);
        socket.bind(zmq_endpoint);
        std::cout << "Bound ZMQ socket to: " << zmq_endpoint << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ bind error: " << e.what() << std::endl;
        return -1;
    }

    // 4. Эмуляция бесконечного потока кадров
    uint64_t frame_counter = 0;
    std::cout << "Starting frame emulation loop..." << std::endl;

    while (true) {
        // Создаём копию изображения (на случай, если ImageStructure изменяет данные)
        cv::Mat frame = image.clone();

        // Сериализуем как в реальной версии
        ImageStructure is(frame, frame_counter);
        auto serialized = is.serialize();

        zmq::message_t msg(serialized.size());
        memcpy(msg.data(), serialized.data(), serialized.size());

        // Отправляем неблокирующе (как в real)
        auto result = socket.send(msg, zmq::send_flags::dontwait);
        if (!result.has_value()) {
            // Буфер переполнен — просто пропускаем кадр (как в real)
            if (frame_counter % 50 == 0) {
                std::cout << "- [ WARN ] Buffer full, dropping frame " << frame_counter << std::endl;
            }
        } else {
            std::cout << "- [ OK ] Sent frame: " << frame_counter
                      << " Size: " << frame.cols << "x" << frame.rows
                      << " Serialized: " << serialized.size() << " bytes" << std::endl;
        }

        frame_counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    }

    return 0;
}