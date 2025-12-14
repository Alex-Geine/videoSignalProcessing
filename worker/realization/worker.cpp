#include <iostream>
#include <thread>
#include <chrono>
#include "utils.h"


// Функция для пастеризации (квантования цвета)
cv::Mat applyColorQuantization(const cv::Mat& image, int levels = 8) {
    if (image.empty()) return cv::Mat();
    
    cv::Mat result = image.clone();
    
    // Если изображение уже в оттенках серого
    if (image.channels() == 1) {
        for (int i = 0; i < result.rows; i++) {
            for (int j = 0; j < result.cols; j++) {
                uchar pixel = result.at<uchar>(i, j);
                // Квантование для градаций серого
                uchar new_value = (pixel / (256 / levels)) * (256 / levels);
                result.at<uchar>(i, j) = new_value;
            }
        }
    } 
    // Если цветное изображение (3 канала)
    else if (image.channels() == 3) {
        for (int i = 0; i < result.rows; i++) {
            for (int j = 0; j < result.cols; j++) {
                cv::Vec3b pixel = result.at<cv::Vec3b>(i, j);
                // Квантование для каждого цветового канала
                for (int ch = 0; ch < 3; ch++) {
                    pixel[ch] = (pixel[ch] / (256 / levels)) * (256 / levels);
                }
                result.at<cv::Vec3b>(i, j) = pixel;
            }
        }
    }
    
    return result;
}


// Функция для выделения контуров
cv::Mat applyEdgeDetection(const cv::Mat& image) {
    if (image.empty()) return cv::Mat();
    
    cv::Mat grayscale, edges;
    
    // Если изображение цветное, конвертируем в оттенки серого
    if (image.channels() == 3) {
        cv::cvtColor(image, grayscale, cv::COLOR_BGR2GRAY);
    } else {
        grayscale = image.clone();
    }
    
    // Применяем размытие для уменьшения шума
    cv::GaussianBlur(grayscale, grayscale, cv::Size(3, 3), 0);
    
    // Детектор Кэнни для выделения контуров
    cv::Canny(grayscale, edges, 50, 150);
    
    // Инвертируем: контуры становятся белыми (255) на чёрном фоне (0)
    cv::bitwise_not(edges, edges);
    
    // Конвертируем в 3 канала для совместимости с цветным изображением
    cv::Mat edges_bgr;
    cv::cvtColor(edges, edges_bgr, cv::COLOR_GRAY2BGR);
    
    return edges_bgr;
}


cv::Mat applyEffect(const cv::Mat& image, int levels = 8) {
    if (image.empty()) return cv::Mat();
    
    cv::Mat eff = applyColorQuantization(image, levels);
    cv::Mat edges_mask = applyEdgeDetection(image);
    cv::Mat result = eff.clone();
    
    // Проходим по всем пикселям
    for (int i = 0; i < result.rows; i++) {
        for (int j = 0; j < result.cols; j++) {
            // Если в маске контуров пиксель чёрный (0,0,0) - делаем контур чёрным
            cv::Vec3b edge_pixel = edges_mask.at<cv::Vec3b>(i, j);
            if (edge_pixel[0] == 0 && edge_pixel[1] == 0 && edge_pixel[2] == 0) {
                result.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 0); // Чёрный цвет
            }
        }
    }
    
    return result;
}


cv::Mat combineImagesSideBySide(const cv::Mat& left_image, const cv::Mat& right_image) {
    if (left_image.empty()) return right_image.clone();
    if (right_image.empty()) return left_image.clone();
    
    cv::Mat right_resized;
    if (left_image.rows != right_image.rows) {
        double scale_factor = (double)left_image.rows / right_image.rows;
        cv::resize(right_image, right_resized, 
                   cv::Size((int)(right_image.cols * scale_factor), left_image.rows));
    } else {
        right_resized = right_image.clone();
    }

    int total_width = left_image.cols + right_resized.cols;
    int height = left_image.rows;
    
    cv::Mat combined;

    if (left_image.channels() == 3 && right_resized.channels() == 3) {
        combined = cv::Mat(height, total_width, CV_8UC3, cv::Scalar(0, 0, 0));
        left_image.copyTo(combined(cv::Rect(0, 0, left_image.cols, height)));
        right_resized.copyTo(combined(cv::Rect(left_image.cols, 0, right_resized.cols, height)));
    }
    else if (left_image.channels() == 1 && right_resized.channels() == 1) {
        combined = cv::Mat(height, total_width, CV_8UC1, cv::Scalar(0));
        left_image.copyTo(combined(cv::Rect(0, 0, left_image.cols, height)));
        right_resized.copyTo(combined(cv::Rect(left_image.cols, 0, right_resized.cols, height)));
    }
    else {
        cv::Mat left_color, right_color;
        
        if (left_image.channels() == 1) {
            cv::cvtColor(left_image, left_color, cv::COLOR_GRAY2BGR);
        } else {
            left_color = left_image.clone();
        }
        
        if (right_resized.channels() == 1) {
            cv::cvtColor(right_resized, right_color, cv::COLOR_GRAY2BGR);
        } else {
            right_color = right_resized.clone();
        }
        
        combined = cv::Mat(height, total_width, CV_8UC3, cv::Scalar(0, 0, 0));
        left_color.copyTo(combined(cv::Rect(0, 0, left_color.cols, height)));
        right_color.copyTo(combined(cv::Rect(left_color.cols, 0, right_color.cols, height)));

        cv::line(combined, 
                 cv::Point(left_color.cols, 0), 
                 cv::Point(left_color.cols, height), 
                 cv::Scalar(0, 255, 0), 2);
    }
    
    return combined;
}

int main() {
    Utils worker;
    worker.loadConfig();
    
    std::string server_ip = worker.getConfig("server.ip");
    int server_port = std::stoi(worker.getConfig("server.port"));
    std::string output_dir = worker.getConfig("worker.output_dir");
    
    std::cout << "1 Real Worker started..." << std::endl;
    std::cout << "Server: " << server_ip << ":" << server_port << std::endl;
    std::cout << "Output directory: " << output_dir << std::endl;
    
    // Параметр для пастеризации (количество уровней цвета)
    int quantization_levels = 4;
    
    while (true) {
        std::cout << "\n=== Worker cycle ===" << std::endl;
        
        // Подключаемся к серверу
        if (worker.initializeClient(server_ip, server_port)) {
            // 1. Запрашиваем работу
            worker.sendMessage("READY");
            std::cout << "Sent READY to server" << std::endl;
            
            // 2. Получаем изображение от сервера
            cv::Mat original_image = worker.receiveImage();
            if (!original_image.empty()) {
                std::cout << "Image received from server: " 
                          << original_image.cols << "x" << original_image.rows 
                          << ", channels: " << original_image.channels() << std::endl;
                
                // Сохраняем оригинал
                worker.setCurrentImage(original_image);
                std::string original_path = output_dir + "worker_original.bmp";
                worker.saveImage(original_path);
                std::cout << "Original saved: " << original_path << std::endl;
                
                // 3. Обрабатываем изображение: мультипликационный эффект
                std::cout << "Applying cartoon effect (quantization + edges)..." << std::endl;
                std::cout << "Quantization levels: " << quantization_levels << std::endl;
                
                cv::Mat processed_image = applyEffect(original_image, quantization_levels);
                
                std::cout << "Processing completed. Result: " 
                          << processed_image.cols << "x" << processed_image.rows 
                          << ", channels: " << processed_image.channels() << std::endl;
                
                // Сохраняем обработанное изображение
                worker.setCurrentImage(processed_image);
                std::string processed_path = output_dir + "worker_processed.bmp";
                worker.saveImage(processed_path);
                std::cout << "Processed saved: " << processed_path << std::endl;
                
                // 4. Объединяем исходное и обработанное изображения
                std::cout << "Creating combined image..." << std::endl;
                cv::Mat combined_image = combineImagesSideBySide(original_image, processed_image);
                
                worker.setCurrentImage(combined_image);
                std::string combined_path = output_dir + "worker_combined.bmp";
                worker.saveImage(combined_path);
                
                std::cout << "Combined image created: " 
                          << combined_image.cols << "x" << combined_image.rows
                          << ", saved: " << combined_path << std::endl;
                
                // 5. Отправляем обработанное изображение обратно серверу
                std::cout << "Sending processed image to server..." << std::endl;
                worker.sendImage(combined_image);
                
                // 6. Ждем подтверждение от сервера
                std::string ack = worker.receiveMessage();
                if (ack == "DONE") {
                    std::cout << "Server confirmed completion" << std::endl;
                } else {
                    std::cout << "Server response: " << ack << std::endl;
                }
            } else {
                std::cout << "Received empty image from server" << std::endl;
            }
        } else {
            std::cout << "Failed to connect to server" << std::endl;
        }
        
        std::cout << "=== Worker cycle completed ===" << std::endl;
        std::cout << "Waiting 3 seconds before next cycle..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
    
    return 0;
}