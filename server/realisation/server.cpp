#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>
#include "server.h"
#include "zmq.hpp"


struct image_header
{
    // starts with 0, 1, 2...
    uint64_t timestamp;

    uint32_t width;
    uint32_t height;
    // uint32_t channels;

    size_t size() {return sizeof(image_header);}
    void* data() {return this;}
};

void Server::process()
{
    zmq::context_t ctx;

    cv::VideoCapture cap(camera_index);

    if (!cap.isOpened())
    {
        std::cerr << "Error: camera is not initialized" << std::endl;
    }

    image_header h;
    h.width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    h.height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    h.timestamp = 0;


    zmq::socket_t worker1(ctx,zmq::socket_type::push);
    zmq::socket_t worker2(ctx,zmq::socket_type::push);
    zmq::socket_t worker3(ctx,zmq::socket_type::push);
    zmq::socket_t worker4(ctx,zmq::socket_type::push);

    std::array<zmq::socket_ref,4> workers{worker1,worker2,worker3,worker4};

    workers[0].connect(utils_.getConfig("worker1.ip"));
    workers[1].connect(utils_.getConfig("worker2.ip"));
    workers[2].connect(utils_.getConfig("worker3.ip"));
    workers[3].connect(utils_.getConfig("worker4.ip"));

    for (size_t i = 0 ; i < workers.size(); ++i)
    {
        if (workers[i].handle() == nullptr) 
            std::cerr << "Error: worker " << i << "is unavailable" << std::endl;
    }

    while (true)
    {
        cv::Mat frame;
        cap >> frame;

        if (frame.empty()) {std::cerr << "Error: empty image" << std::endl; continue;}

        zmq::message_t header(h.data(),h.size());

        size_t frame_size =  frame.total() * frame.elemSize();
        zmq::message_t image_data(frame.data,frame_size,[](void*, void*){}); // makes it zero-copy

        for (auto& socket : workers)
        {
            socket.send(header,zmq::send_flags::sndmore); // multipart message
            socket.send(image_data,zmq::send_flags::none);
        }

        h.timestamp++;
    }    
}

int main()
{
    Server server;
    server.process();
    return 0;
}