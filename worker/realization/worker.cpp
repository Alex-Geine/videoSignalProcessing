// worker/realization/worker.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <zmq.hpp>
#include "ImageStructure.hpp"
#include "utils.h"

// === Ваши функции обработки (без изменений) ===

cv::Mat applyColorQuantization(const cv::Mat& image, int levels = 8) {
    if (image.empty()) return cv::Mat();
    cv::Mat result = image.clone();
    if (image.channels() == 1) {
        for (int i = 0; i < result.rows; ++i)
            for (int j = 0; j < result.cols; ++j) {
                uchar& pixel = result.at<uchar>(i, j);
                pixel = (pixel / (256 / levels)) * (256 / levels);
            }
    } else if (image.channels() == 3) {
        for (int i = 0; i < result.rows; ++i)
            for (int j = 0; j < result.cols; ++j) {
                cv::Vec3b& pixel = result.at<cv::Vec3b>(i, j);
                for (int ch = 0; ch < 3; ++ch)
                    pixel[ch] = (pixel[ch] / (256 / levels)) * (256 / levels);
            }
    }
    return result;
}

cv::Mat applyEdgeDetection(const cv::Mat& image) {
    if (image.empty()) return cv::Mat();
    cv::Mat grayscale, edges;
    if (image.channels() == 3)
        cv::cvtColor(image, grayscale, cv::COLOR_BGR2GRAY);
    else
        grayscale = image.clone();
    cv::GaussianBlur(grayscale, grayscale, cv::Size(3, 3), 0);
    cv::Canny(grayscale, edges, 50, 150);
    cv::bitwise_not(edges, edges);
    cv::Mat edges_bgr;
    cv::cvtColor(edges, edges_bgr, cv::COLOR_GRAY2BGR);
    return edges_bgr;
}

cv::Mat applyEffect(const cv::Mat& image, int levels = 8) {
    if (image.empty()) return cv::Mat();
    cv::Mat eff = applyColorQuantization(image, levels);
    cv::Mat edges_mask = applyEdgeDetection(image);
    cv::Mat result = eff.clone();
    for (int i = 0; i < result.rows; ++i)
        for (int j = 0; j < result.cols; ++j) {
            cv::Vec3b edge_pixel = edges_mask.at<cv::Vec3b>(i, j);
            if (edge_pixel == cv::Vec3b(0, 0, 0))
                result.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 0);
        }
    return result;
}

cv::Mat combineImagesSideBySide(const cv::Mat& left_image, const cv::Mat& right_image) {
    if (left_image.empty()) return right_image.clone();
    if (right_image.empty()) return left_image.clone();
    
    cv::Mat right_resized = right_image;
    if (left_image.rows != right_image.rows) {
        double scale = (double)left_image.rows / right_image.rows;
        cv::resize(right_image, right_resized, cv::Size((int)(right_image.cols * scale), left_image.rows));
    }

    int total_width = left_image.cols + right_resized.cols;
    int height = left_image.rows;
    cv::Mat combined(height, total_width, CV_8UC3, cv::Scalar(0, 0, 0));

    cv::Mat left_color = (left_image.channels() == 1) ? 
        (cv::Mat)left_image : left_image;
    if (left_image.channels() == 1)
        cv::cvtColor(left_image, left_color, cv::COLOR_GRAY2BGR);

    cv::Mat right_color = (right_resized.channels() == 1) ?
        (cv::Mat)right_resized : right_resized;
    if (right_resized.channels() == 1)
        cv::cvtColor(right_resized, right_color, cv::COLOR_GRAY2BGR);

    left_color.copyTo(combined(cv::Rect(0, 0, left_color.cols, height)));
    right_color.copyTo(combined(cv::Rect(left_color.cols, 0, right_color.cols, height)));
    cv::line(combined, cv::Point(left_color.cols, 0), cv::Point(left_color.cols, height), cv::Scalar(0, 255, 0), 2);
    return combined;
}

// === Основная функция ===

int main() {
    std::cout << "=== WORKER (REAL MODE) STARTED ===" << std::endl;

#ifdef DEBUG
    std::cout << "DEBUG MODE ENABLED" << std::endl;
#endif

    // 1. Загрузка конфигурации
    Utils config;
    config.loadConfig();

    std::string input_endpoint = config.getConfig("worker.server_endpoint");        // "tcp://localhost:5555"
    std::string output_endpoint = config.getConfig("worker.postprocessor_endpoint"); // "tcp://localhost:5557"
    std::string output_dir = config.getConfig("worker.output_dir");
    int quantization_levels = 4; // можно вынести в конфиг

    std::filesystem::create_directories(output_dir);

    // 2. ZeroMQ
    zmq::context_t ctx(1);
    zmq::socket_t input_socket(ctx, zmq::socket_type::pull);
    zmq::socket_t output_socket(ctx, zmq::socket_type::push);
    
    input_socket.set(zmq::sockopt::rcvhwm, 2);
    output_socket.set(zmq::sockopt::sndhwm, 2);

    input_socket.connect(input_endpoint);
    output_socket.connect(output_endpoint);

    std::cout << "Connected to server: " << input_endpoint << std::endl;
    std::cout << "Connected to postprocessor: " << output_endpoint << std::endl;
    std::cout << "Output dir: " << output_dir << std::endl;

    // 3. Основной цикл
    while (true) {
        zmq::message_t msg;
        auto result = input_socket.recv(msg, zmq::recv_flags::none);
        if (!result.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Десериализация
        ImageStructure is;
        if (!is.deserialize(msg)) {
            std::cerr << "Deserialization failed" << std::endl;
            continue;
        }

        cv::Mat original = is.getImage();
        uint64_t frame_id = is.getFrameId();

        if (original.empty()) {
            std::cerr << "Received empty image" << std::endl;
            continue;
        }

        std::cout << "- [ OK ] Received frame " << frame_id
                  << " (" << original.cols << "x" << original.rows
                  << ", ch=" << original.channels() << ")" << std::endl;

        // Сохраняем оригинал
        // cv::imwrite(output_dir + "worker_original_" + std::to_string(frame_id) + ".bmp", original);

        // Обработка
        cv::Mat processed = applyEffect(original, quantization_levels);
        // cv::imwrite(output_dir + "worker_processed_" + std::to_string(frame_id) + ".bmp", processed);

        cv::Mat combined = combineImagesSideBySide(original, processed);
        // cv::imwrite(output_dir + "worker_combined_" + std::to_string(frame_id) + ".bmp", combined);

        // Отправка в postprocessor
        ImageStructure out_is(combined, frame_id);
        auto out_data = out_is.serialize();

        zmq::message_t out_msg(out_data.size());
        std::memcpy(out_msg.data(), out_data.data(), out_data.size());

        auto send_res = output_socket.send(out_msg, zmq::send_flags::none);
        if (!send_res.has_value()) {
            std::cerr << "Warning: failed to send frame " << frame_id << std::endl;
        }

        std::cout << "- [ OK ] Sent processed frame " << frame_id << " to postprocessor" << std::endl;
    }

    return 0;
}