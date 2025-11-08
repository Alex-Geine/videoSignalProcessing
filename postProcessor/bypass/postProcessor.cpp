#include <iostream>
#include <thread>
#include <chrono>
#include "utils.h"

int main() {
    Utils postprocessor;
    postprocessor.loadConfig();
    
    std::string ip = postprocessor.getConfig("postprocessor.ip");
    int port = std::stoi(postprocessor.getConfig("postprocessor.port"));
    std::string output_dir = postprocessor.getConfig("postprocessor.output_dir");
    std::string proc_prefix = postprocessor.getConfig("postprocessor.processed_prefix");
    std::string bare_prefix = postprocessor.getConfig("postprocessor.bare_prefix");
    
    static int image_counter = 1;
    
    if (!postprocessor.initializeServer(ip, port)) {
        return -1;
    }
    
    std::cout << "PostProcessor started. Waiting for server..." << std::endl;
    
    while (true) {
        std::cout << "\nWaiting for server..." << std::endl;
        
        // Ждем запрос от сервера
        std::string request = postprocessor.receiveMessage();
        if (request == "READY") {
            std::cout << "Server is ready. Receiving images..." << std::endl;
            
            // Отправляем подтверждение, что готовы получать изображения
            postprocessor.sendMessage("SEND_FIRST_IMAGE");
            
            // Получаем первое изображение (обработанное)
            cv::Mat processed_image = postprocessor.receiveImage();
            if (!processed_image.empty()) {
                postprocessor.setCurrentImage(processed_image);
                std::string proc_filename = output_dir + proc_prefix + std::to_string(image_counter) + ".bmp";
                postprocessor.saveImage(proc_filename);
                std::cout << "Saved processed image: " << proc_filename << std::endl;
                
                // Подтверждаем получение первого изображения
                postprocessor.sendMessage("SEND_SECOND_IMAGE");
                
                // Получаем второе изображение (оригинальное)
                cv::Mat original_image = postprocessor.receiveImage();
                if (!original_image.empty()) {
                    postprocessor.setCurrentImage(original_image);
                    std::string bare_filename = output_dir + bare_prefix + std::to_string(image_counter) + ".bmp";
                    postprocessor.saveImage(bare_filename);
                    std::cout << "Saved original image: " << bare_filename << std::endl;
                    
                    // Подтверждаем завершение
                    postprocessor.sendMessage("DONE");
                    
                    std::cout << "Postprocessing completed. Total: " << image_counter << std::endl;
                    image_counter++;
                }
            }
        }
    }
    
    return 0;
}