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
    std::string getVersion();
};
#endif // _UTILS_H_