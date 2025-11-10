#ifndef _SERVER_H_
#define _SERVER_H_

// OpenCV основные заголовки
#include "utils.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>  // Добавляем для cvtColor
#include <opencv2/imgcodecs.hpp> // Добавляем для imread/imwrite
#include <opencv2/highgui.hpp>   // Добавляем для окон

// #include <zmq.hpp>

// Server realisation Class
class Server
{
    std::string ip;
    int port;
    int camera_index = 0;

    Utils utils_;
public:

    Server()
    {
        utils_.loadConfig();
        ip = utils_.getConfig("server.ip");
        // ip = "localhost";
        port = std::stoi(utils_.getConfig("server.port"));
        camera_index = 0;
    }






    // Server process function
    void process();
};

#endif // _SERVER_H_