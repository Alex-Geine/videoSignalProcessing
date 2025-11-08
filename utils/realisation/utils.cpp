#include <iostream>
#include "utils.h"

void Utils::process()
{
    std::cout << "Utils REAL implementation: Processing video data..." << std::endl;
}

int main()
{
    Utils utils;
    utils.process();
    return 0;
}