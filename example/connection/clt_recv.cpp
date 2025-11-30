#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <zmq.hpp>

#include "ImageStructure.hpp"

using namespace cv;
using namespace std;

int main()
{
    zmq::context_t ctx;
    zmq::socket_t socket(ctx,zmq::socket_type::pull);
    socket.connect("tcp://127.0.0.1:5555");
    zmq::message_t msg;

    namedWindow("test",cv::WindowFlags::WINDOW_FREERATIO);
    while(true)
    {
        auto rcv = socket.recv(msg);
        
        auto s = msg.to_string();
        
        Mat image;
        ImageStructure is(image);
        is.deserialize(s);
        
        imshow("test",image);
        // sleep(1);
        waitKey(1000/24);
    }
        
}
