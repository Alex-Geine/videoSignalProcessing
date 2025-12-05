#include <iostream>
#include <stdio.h>
//#include "server.h"
#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>
#include <filesystem>
#include <zmq.hpp>
#include "ImageStructure.hpp"



class Capturer
{
private:
	cv::VideoCapture cap;
	uint64_t frame_counter;
	std::filesystem::path temp_dir;

	zmq::context_t zmq_ctx;
	zmq::socket_t socket;

public:
	Capturer() : frame_counter(0)
		, zmq_ctx(1)
		, socket(zmq_ctx, zmq::socket_type::push)
	{
		std::cout << "=== Capturer Initialization ===" << std::endl;
		temp_dir = "./camera_capture";
		std::filesystem::create_directories(temp_dir);
		init_camera();
		init_zmq();
		std::cout << "======================================================" << std::endl;
	}

private:
	void init_camera()
	{
		std::cout << "Searching for camera..." << std::endl;
		for (int i = 0; i < 10; i++)
		{
			cap.open(i);
			if (cap.isOpened())
			{
				cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
				cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
				cap.set(cv::CAP_PROP_FPS, 30);
				std::cout << "- [ OK ] Camera found at ID: " << i << std::endl;
				return;
			}
		}
		throw std::runtime_error("- [FAIL] No camera found!");
	}


	void init_zmq()
	{
		try {
			
			int send_buffer_limit = 100;
			socket.set(zmq::sockopt::sndhwm, send_buffer_limit);

			socket.set(zmq::sockopt::linger, 0);
			socket.set(zmq::sockopt::immediate, 1);

			socket.bind("tcp://localhost:5555");

			std::cout << "- [ OK ] ZMQ socket bound" << std::endl;
			std::cout << "- [ INFO ] Send buffer limit (HWM): " << send_buffer_limit << " messages" << std::endl;
		}
		catch (const zmq::error_t& e) {
			throw std::runtime_error(std::string("- [FAIL] ZMQ bind error: ") + e.what());
		}
	}


public:
	void run()
	{
		std::cout << "=== Capturer Started ===" << std::endl;
		std::cout << "Displaying camera feed..." << std::endl;

		cv::Mat frame;
		int dropped_frames = 0;

		while (true)
		{
			if (!cap.read(frame) || frame.empty())
				// if (cap.read(frame) && !frame.empty())
			{
				std::cout << "- [FAIL] Failed to grab frame" << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}


			// Seriliazation
			ImageStructure is1(frame, frame_counter);
			auto serialized = is1.serialize();

			zmq::message_t msg(serialized.size());
			memcpy(msg.data(), serialized.data(), serialized.size());

			zmq::send_flags flags = zmq::send_flags::dontwait;
			auto result = socket.send(msg, flags);

			if (!result.has_value()) {
				dropped_frames++;
				if (dropped_frames % 50 == 0) {
					std::cout << "- [ WARN ] Buffer full, dropped " << dropped_frames << " frames total" << std::endl;
				}
				continue;
			}

			std::cout << "- [ OK ] Sent frame: " << frame_counter
				<< " Size: " << frame.cols << "x" << frame.rows
				<< " Channels: " << frame.channels()
				<< " Serialized size: " << serialized.size() << " bytes" << std::endl;


			//cv::imshow("Capturer - Camera Feed", frame);

			if (frame_counter) {
				std::string filename = (temp_dir / ("frame_" + std::to_string(frame_counter) + ".jpg")).string();
				cv::imwrite(filename, frame);
				//std::cout << "  Saved: " << filename << std::endl;
			}


			std::cout << "- [ OK ] Captured frame: " << frame_counter++
				<< " Size: " << frame.cols << "x" << frame.rows
				<< " Channels: " << frame.channels() << std::endl;

			//if (cv::waitKey(1) == 27) break;
		}

		cap.release();
		cv::destroyAllWindows();
	}
};

//void Server::process()
//{
//    std::cout << "Server REAL implementation: Processing video data..." << std::endl;
//}

int main()
{
	try
	{
		Capturer capturer;
		capturer.run();
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cout << "- [FAIL] Capturer error: " << e.what() << std::endl;
		return -1;
	}
	//Server server;
	//server.process();
	//return 0;
}