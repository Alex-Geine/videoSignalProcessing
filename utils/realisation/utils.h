#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
#include <memory>

class Utils {
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;

public:
    Utils();
    ~Utils();
    
    void processConfig();
    void sendMessage(const std::string& message);
    std::string receiveMessage();
    bool connect(const std::string& address);
    bool bind(const std::string& address);
    
    // OpenCV методы
    bool initializeCamera(int camera_index = 0);
    void processVideoFrame();
    void applyImageFilter();
    void displayImage();
    
    std::string getVersion();
};

#endif // _UTILS_H_