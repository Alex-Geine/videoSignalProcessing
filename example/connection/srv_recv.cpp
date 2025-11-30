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
    zmq::socket_t socket(zmq_ctx, zmq::socket_type::pull);
    socket.bind("tcp://127.0.0.1:5555");
    
    // Mat image = imread("test.png");
    Mat image;
    
    while (true)
    {
        zmq::message_t msg;
        auto rcv = socket.recv(msg, zmq::recv_flags::none);

        ImageStructure is1(image);

        auto s = msg.to_string();
        is1.deserialize(s);
        
        imshow("test",image);
        // sleep(1);
        waitKey(1000/24);
    }
}
