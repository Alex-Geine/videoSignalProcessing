#include <iostream>
#include "postProcessor.h"

void PostProcessor::process()
{
    std::cout << "PostProcessor BYPASS implementation: Processing video data..." << std::endl;
}

int main()
{
    PostProcessor postProcessor;
    postProcessor.process();
    return 0;
}