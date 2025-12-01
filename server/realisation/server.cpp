#include <iostream>
#include <stdio.h>
//#include "server.h"
#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>

// Весь ниже закомментированный код для ваших дальнейших добавлений

//#include <zmq.hpp>
//#include "ImageStructure.hpp"



class Capturer
{
private:
	cv::VideoCapture cap;
	uint64_t frame_counter;
	/*
	zmq::context_t zmq_ctx;
	zmq::socket_t socket;
	*/
public:
	Capturer() : frame_counter(0)
		//, zmq_ctx(1)
		//, socket(zmq_ctx, zmq::socket_type::push)
	{
		std::cout << "=== Capturer Initialization ===" << std::endl;
		init_camera();
		//init_zmq();
		std::cout << "======================================================" << std::endl;
	}

private:
	void init_camera()
	{
		std::cout << "Searching for camera..." << std::endl;
		for (int i = 0; i < 10; i++) // Поиск доступной камеры (0-9)
		{
			cap.open(i); // Попытка открыть камеру с текущим ID
			if (cap.isOpened())
			{
				// Установка параметров камеры
				cap.set(cv::CAP_PROP_FRAME_WIDTH, 640); // Ширина кадра
				cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480); // Высота кадра
				cap.set(cv::CAP_PROP_FPS, 30); // Частота кадров
				std::cout << "- [ OK ] Camera found at ID: " << i << std::endl;
				return;
			}
		}
		throw std::runtime_error("- [FAIL] No camera found!");
	}

	/*
	void init_zmq()
	{
	   try {
			socket.bind("tcp://127.0.0.1:5555");
			std::cout << "- [ OK ] ZMQ socket bound to tcp://127.0.0.1:5555" << std::endl;
		}
		catch (const zmq::error_t& e) {
			throw std::runtime_error(std::string("- [FAIL] ZMQ bind error: ") + e.what());
		}
	*/


public:
	void run()
	{
		std::cout << "=== Capturer Started ===" << std::endl;
		std::cout << "Displaying camera feed..." << std::endl;

		cv::Mat frame; // Переменная для хранения кадра
		while (true) // Чтение кадра с камеры
		{
			//if (!cap.read(frame) || frame.empty())
			if (cap.read(frame) && !frame.empty())
			{
				std::cout << "- [FAIL] Failed to grab frame" << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Пауза при ошибке чтения кадра
				continue;
			}

			/*
			// 1. Сериализация кадра
			ImageStructure is1(frame, frame_counter);
			auto serialized = is1.serialize();

			// 2. Отправка через ZMQ
			zmq::message_t msg(serialized.size());
			memcpy(msg.data(), serialized.data(), serialized.size());
			socket.send(msg, zmq::send_flags::none);

			// 3. Вывод информации о кадре (Удалить ниже вывод о [ OK ] Captured frame)
			std::cout << "- [ OK ] Sent frame: " << frame_counter
				<< " Size: " << frame.cols << "x" << frame.rows
				<< " Channels: " << frame.channels()
				<< " Serialized size: " << serialized.size() << " bytes" << std::endl;
			*/

			// Отображение кадра в окне
			cv::imshow("Capturer - Camera Feed", frame);

			// Вывод информации о кадре в cmd
			std::cout << "- [ OK ] Captured frame: " << frame_counter++
				<< " Size: " << frame.cols << "x" << frame.rows
				<< " Channels: " << frame.channels() << std::endl;

			// Не работает, завершай через Ctrl+C
			if (cv::waitKey(1) == 27) break; // Проверка нажатия ESC для выхода
		}

		cap.release(); // Освобождение ресурсов камеры
		cv::destroyAllWindows(); // Закрытие всех окон
		std::cout << "=== Capturer Stopped ===" << std::endl;
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
		Capturer capturer; // Создание объекта камеры
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





