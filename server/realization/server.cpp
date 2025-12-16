// server/realization/server.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <zmq.hpp>
#include "ImageStructure.hpp"
#include "utils.h"

int main() {
    std::cout << "=== SERVER (REAL MODE) STARTED ===" << std::endl;

#ifdef DEBUG
    std::cout << "DEBUG MODE ENABLED" << std::endl;
#endif

    // 1. Загружаем конфиг (для zmq_port, возможно других параметров)
    Utils server;
    server.loadConfig();
    std::string zmq_endpoint = "tcp://*:" + server.getConfig("server.zmq_port"); // например, "5555"

    // 2. Инициализируем камеру
    cv::VideoCapture cap;
    std::cout << "Searching for camera..." << std::endl;

    bool camera_found = false;
    for (int i = 0; i < 10; ++i) {
        cap.open(i, cv::CAP_ANY);
        if (cap.isOpened()) {
            // Попробуем считать хотя бы один кадр
            cv::Mat test_frame;
            if (cap.read(test_frame) && !test_frame.empty()) {
                cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
                cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
                cap.set(cv::CAP_PROP_FPS, 30);
                std::cout << "- [ OK ] Camera found at ID: " << i << std::endl;
                camera_found = true;
                break;
            } else {
                cap.release();
            }
        }
    }

    if (!camera_found) {
        std::cerr << "- [FAIL] No working camera found!" << std::endl;
        return -1;
    }

    // 3. Инициализируем ZeroMQ
    zmq::context_t ctx(1);
    zmq::socket_t socket(ctx, zmq::socket_type::push);

    try {
        socket.set(zmq::sockopt::sndhwm, 2);   // Лимит буфера отправки
        socket.set(zmq::sockopt::linger, 0);     // Не ждать при закрытии
        socket.bind(zmq_endpoint);
        std::cout << "- [ OK ] ZMQ socket bound to: " << zmq_endpoint << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ bind error: " << e.what() << std::endl;
        cap.release();
        return -1;
    }

    // 4. Основной цикл захвата и отправки
    std::cout << "=== Camera streaming started ===" << std::endl;
    uint64_t frame_counter = 0;
    cv::Mat frame;

    while (true) {
        if (!cap.read(frame) || frame.empty()) {
            std::cerr << "- [FAIL] Failed to grab frame from camera" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Сериализуем кадр
        ImageStructure is(frame, frame_counter);
        auto serialized = is.serialize();

        zmq::message_t msg(serialized.size());
        std::memcpy(msg.data(), serialized.data(), serialized.size());

        // Отправляем неблокирующе
        auto result = socket.send(msg, zmq::send_flags::dontwait);
        if (!result.has_value()) {
            if (frame_counter % 50 == 0) {
                std::cout << "- [ WARN ] Buffer full, dropped frame " << frame_counter << std::endl;
            }
        } else {
            std::cout << "- [ OK ] Sent frame: " << frame_counter
                      << " (" << frame.cols << "x" << frame.rows << ")"
                      << " serialized size: " << serialized.size() << " bytes" << std::endl;
        }

        frame_counter++;
        // cv::waitKey(1) не нужен — мы не показываем окно
    }

    // Освобождение ресурсов (достижимо только при graceful shutdown)
    cap.release();
    return 0;
}