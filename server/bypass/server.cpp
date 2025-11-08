#include <iostream>
#include "server.h"

void Server::process()
{
    std::cout << "Server BYPASS implementation: Processing video data..." << std::endl;
}

int main()
{
    Server server;
    server.process();
    return 0;
}