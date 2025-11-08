#include <iostream>
#include <thread>
#include <chrono>
#include "utils.h"

cv::Mat convertToGrayscale(const cv::Mat& color_image) {
    if (color_image.channels() != 3) {
        return color_image.clone();
    }
    
    cv::Mat grayscale(color_image.rows, color_image.cols, CV_8UC1);
    
    for (int i = 0; i < color_image.rows; i++) {
        for (int j = 0; j < color_image.cols; j++) {
            cv::Vec3b pixel = color_image.at<cv::Vec3b>(i, j);
            grayscale.at<uchar>(i, j) = 
                0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0];
        }
    }
    
    return grayscale;
}

int main() {
    Utils worker;
    worker.loadConfig();
    
    std::string server_ip = worker.getConfig("server.ip");
    int server_port = std::stoi(worker.getConfig("server.port"));
    std::string output_dir = worker.getConfig("worker.output_dir");
    
    std::cout << "Worker started..." << std::endl;
    
    while (true) {
        std::cout << "\n=== Worker cycle ===" << std::endl;
        
        // Подключаемся к серверу
        if (worker.initializeClient(server_ip, server_port)) {
            // 1. Запрашиваем работу
            worker.sendMessage("READY");
            
            // 2. Получаем изображение от сервера
            cv::Mat original_image = worker.receiveImage();
            if (!original_image.empty()) {
                std::cout << "Image received from server: " 
                          << original_image.cols << "x" << original_image.rows << std::endl;
                
                // Сохраняем оригинал
                worker.setCurrentImage(original_image);
                worker.saveImage(output_dir + "worker_original.bmp");
                
                // 3. Обрабатываем изображение
                cv::Mat processed_image = convertToGrayscale(original_image);
                
                // Сохраняем обработанное
                worker.setCurrentImage(processed_image);
                worker.saveImage(output_dir + "worker_processed.bmp");
                
                std::cout << "Image processing completed" << std::endl;
                
                // 4. Отправляем обработанное изображение обратно серверу
                worker.sendImage(processed_image);
                
                // 5. Ждем подтверждение от сервера
                std::string ack = worker.receiveMessage();
                if (ack == "DONE") {
                    std::cout << "Server confirmed completion" << std::endl;
                }
            }
        }
        
        std::cout << "=== Worker cycle completed ===" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
    
    return 0;
}