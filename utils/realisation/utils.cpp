#include <iostream>
#include "utils.h"
#include <yaml-cpp/yaml.h>

void Utils::processConfig()
{
    std::cout << "Utils REAL implementation with embedded yaml-cpp" << std::endl;
    
    try {
        YAML::Node config = YAML::Load("{name: 'real', version: 1.0}");
        std::cout << "Config name: " << config["name"].as<std::string>() << std::endl;
    } catch (const YAML::Exception& e) {
        std::cout << "YAML error: " << e.what() << std::endl;
    }
}

int main() {
    Utils utils;
    utils.processConfig();
    return 0;
}