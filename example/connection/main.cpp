#include <array>
#include <iostream>

#include "opencv2/videoio.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <zmq.h>

using namespace cv;
using namespace std;


int send()
{

}






int main(int, char**){
    Mat image;
    VideoCapture cap;

    if (!cap.open(0))
    {
        cerr << "Camera is not available" << endl;
    }

    cap >> image;

    // image = imread( argv[1], IMREAD_COLOR );
 
    if ( !image.data )
    {
        printf("No image data \n");
        return -1;
    }
    // namedWindow("Display Image", WindowFlags::WINDOW_FREERATIO );
    // imshow("Display Image", image);
 
    waitKey(0);
 
    return 0;
}
