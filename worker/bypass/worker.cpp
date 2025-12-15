// worker/bypass/worker.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <zmq.hpp>
#include "ImageStructure.hpp"
#include "utils.h"

cv::Mat convertToGrayscale(const cv::Mat& color_image) {
    if (color_image.channels() != 3) {
        return color_image.clone();
    }
    cv::Mat grayscale;
    // Используем OpenCV вместо ручного цикла (быстрее и надёжнее)
    cv::cvtColor(color_image, grayscale, cv::COLOR_BGR2GRAY);
    return grayscale;
}

int main() {
    std::cout << "=== WORKER (BYPASS MODE) STARTED ===" << std::endl;

#ifdef DEBUG
    std::cout << "DEBUG MODE ENABLED" << std::endl;
#endif

    // 1. Загружаем конфиг
    Utils config;
    config.loadConfig();

    std::string input_endpoint = "tcp://localhost:" + config.getConfig("server.zmq_port"); // "5555"
    std::string output_endpoint = "tcp://localhost:" + config.getConfig("postprocessor.zmq_port"); // например, "5556"
    std::string output_dir = config.getConfig("worker.output_dir");

    // Создаём директорию вывода
    std::filesystem::create_directories(output_dir);

    // 2. ZeroMQ сокеты
    zmq::context_t ctx(1);

    // PULL от сервера
    zmq::socket_t input_socket(ctx, zmq::socket_type::pull);
    input_socket.connect(input_endpoint);

    // PUSH в postprocessor
    zmq::socket_t output_socket(ctx, zmq::socket_type::push);
    output_socket.connect(output_endpoint);

    std::cout << "Connected to server: " << input_endpoint << std::endl;
    std::cout << "Connected to postprocessor: " << output_endpoint << std::endl;

    // 3. Основной цикл обработки
    while (true) {
        // Получаем сообщение от сервера
        zmq::message_t msg;
        auto result = input_socket.recv(msg, zmq::recv_flags::none);
        if (!result.has_value() || msg.size() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Десериализуем
        std::vector<uint8_t> data(static_cast<uint8_t*>(msg.data()), 
                                  static_cast<uint8_t*>(msg.data()) + msg.size());
        ImageStructure is;
        if (!is.deserialize(data)) {
            std::cerr << "Failed to deserialize incoming frame" << std::endl;
            continue;
        }

        cv::Mat original = is.getImage();
        uint64_t frame_id = is.getFrameId();

        if (original.empty()) {
            std::cerr << "Received empty image" << std::endl;
            continue;
        }

        std::cout << "- [ OK ] Received frame: " << frame_id
                  << " (" << original.cols << "x" << original.rows << ")" << std::endl;

        // Сохраняем оригинал
        cv::imwrite(output_dir + "worker_original_" + std::to_string(frame_id) + ".bmp", original);

        // Обрабатываем
        cv::Mat processed = convertToGrayscale(original);
        cv::imwrite(output_dir + "worker_processed_" + std::to_string(frame_id) + ".bmp", processed);

        // Отправляем в postprocessor
        ImageStructure out_is(processed, frame_id);
        auto out_data = out_is.serialize();

        zmq::message_t out_msg(out_data.size());
        memcpy(out_msg.data(), out_data.data(), out_data.size());

        output_socket.send(out_msg, zmq::send_flags::none);

        std::cout << "- [ OK ] Sent processed frame " << frame_id << " to postprocessor" << std::endl;
    }

    return 0;
}