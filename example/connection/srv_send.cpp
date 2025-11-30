#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include "ImageStructure.hpp"
#include <thread>
#include <zmq.hpp>

using namespace cv;
using namespace std;

int main()
{
    zmq::context_t zmq_ctx;
    zmq::socket_t socket(zmq_ctx, zmq::socket_type::push);
    socket.bind("tcp://127.0.0.1:5555");
    
    // Mat image = imread("test.png");
    Mat image;
    VideoCapture cap("test.gif", CAP_ANY);
    
    while (true)
    {
        cap >> image;

        ImageStructure is1(image);
        auto s = is1.serialize();

        zmq::message_t msg(s.size());
        memcpy(msg.data(), s.c_str(), s.size());
        using namespace std::chrono_literals;
        // this_thread::sleep_for(1s);
        socket.send(msg, zmq::send_flags::none); // Send the serialized data
    }
}
