#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    Utils server;
    server.loadConfig();
    
    // Загружаем конфигурацию
    std::string ip = server.getConfig("server.ip");
    int port = std::stoi(server.getConfig("server.port"));
    std::string input_image = server.getConfig("server.input_image");
    
    // Инициализируем сервер
    if (!server.initializeServer(ip, port))
    {
        return -1;
    }
    
    std::cout << "Server started. Waiting for clients..." << std::endl;
    
    while (true)
    {
        // Ждем запрос от клиента
        std::string request = server.receiveMessage();
    
        if (request == "READY")
        {
            std::cout << "Worker is ready. Sending image..." << std::endl;
            
            // Загружаем и отправляем изображение
            if (server.loadImage(input_image))
            {
                cv::Mat image = server.getCurrentImage();
                if (server.sendImage(image))
                {
                    std::cout << "Image sent to worker successfully" << std::endl;
                }
            }

            // Ждем подтверждение
            std::string ack = server.receiveMessage();
            if (ack == "DONE")
            {
                std::cout << "Worker processing completed" << std::endl;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    return 0;
}