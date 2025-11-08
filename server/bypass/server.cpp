#include <iostream>
#include <thread>
#include <chrono>
#include "utils.h"

int main() {
    Utils server;
    server.loadConfig();
    
    std::string ip = server.getConfig("server.ip");
    int port = std::stoi(server.getConfig("server.port"));
    std::string input_image = server.getConfig("server.input_image");
    
    // Подключаемся к worker и postprocessor
    Utils worker_client;
    Utils pp_client;
    
    worker_client.loadConfig();
    pp_client.loadConfig();
    
    std::string worker_ip = worker_client.getConfig("worker.ip");
    int worker_port = std::stoi(worker_client.getConfig("worker.port"));
    std::string pp_ip = pp_client.getConfig("postprocessor.ip");
    int pp_port = std::stoi(pp_client.getConfig("postprocessor.port"));
    
    if (!server.initializeServer(ip, port)) {
        return -1;
    }
    
    std::cout << "Server started. Waiting for connections..." << std::endl;
    
    while (true) {
        std::cout << "\n=== New processing cycle ===" << std::endl;
        
        // 1. Ждем запрос от клиента
        std::string request = server.receiveMessage();
        if (request == "READY") {
            std::cout << "Client is ready" << std::endl;
            
            // 2. Отправляем изображение клиенту
            if (server.loadImage(input_image)) {
                cv::Mat original_image = server.getCurrentImage();
                server.sendImage(original_image);
                std::cout << "Original image sent to client" << std::endl;
                
                // 3. Ждем обработанное изображение от клиента
                cv::Mat processed_image = server.receiveImage();
                if (!processed_image.empty()) {
                    std::cout << "Processed image received from client" << std::endl;
                    
                    // 4. Отправляем оба изображения в postprocessor
                    if (pp_client.initializeClient(pp_ip, pp_port)) {
                        pp_client.sendMessage("READY");
                        
                        // Ждем подтверждение для отправки первого изображения
                        std::string ack1 = pp_client.receiveMessage();
                        if (ack1 == "SEND_FIRST_IMAGE") {
                            pp_client.sendImage(processed_image);
                            
                            // Ждем подтверждение для отправки второго изображения
                            std::string ack2 = pp_client.receiveMessage();
                            if (ack2 == "SEND_SECOND_IMAGE") {
                                pp_client.sendImage(original_image);
                                
                                // Ждем окончательное подтверждение
                                std::string pp_ack = pp_client.receiveMessage();
                                if (pp_ack == "DONE") {
                                    std::cout << "Postprocessor completed" << std::endl;
                                }
                            }
                        }
                    }
                    
                    // 5. Подтверждаем клиенту
                    server.sendMessage("DONE");
                    std::cout << "Cycle completed successfully" << std::endl;
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    return 0;
}