#include <iostream>
#include "worker.h"

void Worker::process()
{
    std::cout << "Worker BYPASS implementation: Processing video data..." << std::endl;
}

int main()
{
    Worker worker;
    worker.process();
    return 0;
}