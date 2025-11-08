#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    Utils worker;
    worker.loadConfig();
    
    // Загружаем конфигурацию
    std::string server_ip = worker.getConfig("server.ip");
    int         server_port = std::stoi(worker.getConfig("server.port"));
    std::string pp_ip = worker.getConfig("postprocessor.ip");
    int         pp_port = std::stoi(worker.getConfig("postprocessor.port"));
    std::string output_dir = worker.getConfig("worker.output_dir");
    
    // Подключаемся к серверу
    if (!worker.initializeClient(server_ip, server_port))
    {
        return -1;
    }
    
    // Подключаемся к постпроцессору
    Utils pp_client;
    pp_client.loadConfig();
    if (!pp_client.initializeClient(pp_ip, pp_port))
    {
        return -1;
    }
    
    std::cout << "Worker started. Connecting to server and postprocessor..." << std::endl;
    
    while (true)
    {
        // Сообщаем серверу о готовности
        worker.sendMessage("READY");
        
        // Получаем изображение от сервера
        cv::Mat original_image = worker.receiveImage();

        if (!original_image.empty()) {
            std::cout << "Image received from server" << std::endl;
            
            // Сохраняем оригинальное изображение
            worker.setCurrentImage(original_image);
            worker.saveImage(output_dir + "worker_original.bmp");
            
            // Обрабатываем изображение (делаем черно-белым)
            cv::Mat processed_image;
            cv::cvtColor(original_image, processed_image, cv::COLOR_BGR2GRAY);
            
            // Сохраняем обработанное изображение
            worker.setCurrentImage(processed_image);
            worker.saveImage(output_dir + "worker_processed.bmp");
            
            // Отправляем обработанное изображение постпроцессору
            pp_client.sendMessage("READY");
            pp_client.sendImage(processed_image);
            
            // Отправляем оригинальное изображение постпроцессору
            pp_client.sendImage(original_image);
            
            // Подтверждаем серверу
            worker.sendMessage("DONE");
            
            std::cout << "Processing completed. Waiting for postprocessor..." << std::endl;
            
            // Ждем подтверждение от постпроцессора
            std::string pp_ack = pp_client.receiveMessage();
            if (pp_ack == "DONE")
            {
                std::cout << "Postprocessor completed" << std::endl;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
    
    return 0;
}