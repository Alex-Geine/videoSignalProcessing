#include "utils.h"
#include <iostream>

int main() {
    Utils utils;
    utils.processConfig();
    
    // Тестирование камеры и обработки изображений
    utils.processVideoFrame();
    utils.applyImageFilter();
    utils.displayImage();
    
    // Тестирование ZeroMQ
    utils.connect("tcp://localhost:5555");
    utils.sendMessage("Hello from Utils with OpenCV!");
    utils.receiveMessage();
    
    std::cout << "Utils version: " << utils.getVersion() << std::endl;
    return 0;
}