#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
#include <memory>

// OpenCV основные заголовки
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>   // Добавляем для cvtColor
#include <opencv2/imgcodecs.hpp> // Добавляем для imread/imwrite
#include <opencv2/highgui.hpp>   // Добавляем для окон

class Utils
{
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;

public:
    Utils();
    ~Utils();

    // Конфигурация
    void loadConfig(const std::string &config_path = "config.yaml");
    std::string getConfig(const std::string &node_path);

    // Работа с изображениями
    bool loadImage(const std::string &path);
    bool saveImage(const std::string &path);
    cv::Mat getCurrentImage();
    void setCurrentImage(const cv::Mat &image);

    // Сетевое взаимодействие
    bool initializeServer(const std::string &ip, int port);
    bool initializeClient(const std::string &ip, int port);

    // Передача изображений
    bool sendImage(const cv::Mat &image);
    cv::Mat receiveImage();

    // Простые сообщения
    void sendMessage(const std::string &message);
    std::string receiveMessage();

    // Статус
    bool isConnected();
    std::string getVersion();

private:
    std::string serializeImage(const cv::Mat &image);
    cv::Mat deserializeImage(const std::string &data);
};

#endif // _UTILS_H_