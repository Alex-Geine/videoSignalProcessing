#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    Utils postprocessor;
    postprocessor.loadConfig();
    
    // Загружаем конфигурацию
    std::string ip = postprocessor.getConfig("postprocessor.ip");
    int port = std::stoi(postprocessor.getConfig("postprocessor.port"));
    std::string output_dir = postprocessor.getConfig("postprocessor.output_dir");
    std::string proc_prefix = postprocessor.getConfig("postprocessor.processed_prefix");
    std::string bare_prefix = postprocessor.getConfig("postprocessor.bare_prefix");
    
    static int image_counter = 1;
    
    // Инициализируем сервер
    if (!postprocessor.initializeServer(ip, port)) 
    {
        return -1;
    }
    
    std::cout << "Postprocessor started. Waiting for worker..." << std::endl;
    
    while (true)
    {
        // Ждем запрос от worker
        std::string request = postprocessor.receiveMessage();
    
        if (request == "READY")
        {
            std::cout << "Worker is ready. Receiving images..." << std::endl;
            
            // Получаем обработанное изображение
            cv::Mat processed_image = postprocessor.receiveImage();
            if (!processed_image.empty())
            {
                postprocessor.setCurrentImage(processed_image);
                postprocessor.saveImage(output_dir + proc_prefix + std::to_string(image_counter) + ".bmp");
            }
            
            // Получаем оригинальное изображение
            cv::Mat original_image = postprocessor.receiveImage();

            if (!original_image.empty())
            {
                postprocessor.setCurrentImage(original_image);
                postprocessor.saveImage(output_dir + bare_prefix + std::to_string(image_counter) + ".bmp");
            }
            
            // Увеличиваем счетчик
            image_counter++;
            
            // Подтверждаем worker
            postprocessor.sendMessage("DONE");
            
            std::cout << "Postprocessing completed. Saved images with index: " << image_counter - 1 << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    return 0;
}