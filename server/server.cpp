#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>
#include <filesystem>
//#include <zmq.hpp>
//#include "ImageStructure.hpp"

class Capturer
{
private:
	cv::VideoCapture cap; // Объект для захвата видео с камеры
	uint64_t frame_counter; // Счётчик кадров
	std::filesystem::path temp_dir = "./camera_capture";
	int camera_id = 0;
	//int queue_size = 100;
	int cap_frame_width = 640;
	int cap_frame_height = 480;
	int cap_fps = 30;
	int cap_quality = 80;
	//const std::string capturer_bind_addresses = "tcp://localhost:5555";
	/*
	zmq::context_t zmq_ctx; // Контекст ZeroMQ
	zmq::socket_t socket; // Сокет ZeroMQ для отправки данных
	*/
public:
	Capturer() : frame_counter(0)
		//, zmq_ctx(1) // Инициализация контекста ZeroMQ с одним потоком ввода-вывода
		//, socket(zmq_ctx, zmq::socket_type::push) // Инициализация сокета PUSH
	{
		std::cout << "=== Capturer Initialization ===" << std::endl;
		std::filesystem::create_directories(temp_dir);
		init_camera();
		//init_zmq();
		std::cout << "======================================================" << std::endl;
	}


private:
	void init_camera()
	{
		std::cout << "Searching for camera..." << std::endl;
		cap.open(camera_id); // Попытка открыть камеру с текущим ID
		if (cap.isOpened()) // Проверка успешности открытия камеры
		{
			cap.set(cv::CAP_PROP_FRAME_WIDTH, cap_frame_width); // Установка ширины кадра
			cap.set(cv::CAP_PROP_FRAME_HEIGHT, cap_frame_height); // Установка высоты кадра
			cap.set(cv::CAP_PROP_FPS, cap_fps); // Установка частоты кадров
			std::cout << "- [ OK ] Camera found at ID: " << camera_id << std::endl;
			return;
		}
		throw std::runtime_error("- [FAIL] No camera found!");
	}

	/*
	void init_zmq()
	{
		try {
			int send_buffer_limit = queue_size;
			socket.set(zmq::sockopt::sndhwm, send_buffer_limit); // Установка лимита буфера отправки

			socket.set(zmq::sockopt::linger, 0); // Установка нулевого времени ожидания при закрытии сокета
			socket.set(zmq::sockopt::immediate, 1); // Включение немедленной отправки

			socket.bind(capturer_bind_addresses); // Привязка сокета

			std::cout << "- [ OK ] ZMQ socket bound" << std::endl;
			std::cout << "- [ INFO ] Send buffer limit (HWM): " << send_buffer_limit << " messages" << std::endl;
		}
		catch (const zmq::error_t& e) {
			throw std::runtime_error(std::string("- [FAIL] ZMQ bind error: ") + e.what());
		}
	}
	*/


public:
	void run()
	{
		std::cout << "=== Capturer Started ===" << std::endl;
		std::cout << "Displaying camera feed..." << std::endl;

		cv::Mat frame; // Матрица для хранения текущего кадра
		//int dropped_frames = 0; // Счётчик пропущенных кадров

		while (true)
		{
			if (!cap.read(frame) || frame.empty()) // Попытка захватить кадр
			{
				std::cout << "- [FAIL] Failed to grab frame" << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			//// Сериализация кадра
			//ImageStructure is1(frame, frame_counter); // Создание структуры изображения с кадром и номером
			//auto serialized = is1.serialize(); // Сериализация структуры

			//zmq::message_t msg(serialized.size()); // Создание сообщения ZeroMQ
			//memcpy(msg.data(), serialized.data(), serialized.size()); // Копирование данных в сообщение

			//zmq::send_flags flags = zmq::send_flags::dontwait; // Установка флага неблокирующей отправки
			//auto result = socket.send(msg, flags); // Попытка отправить сообщение

			//if (!result.has_value()) { // Проверка, удалось ли отправить сообщение
			//	dropped_frames++; // Увеличение счётчика пропущенных кадров
			//	if (dropped_frames % 50 == 0) {
			//		std::cout << "- [ WARN ] Buffer full, dropped " << dropped_frames << " frames total" << std::endl; // Каждые 50 пропущенных кадров выводить предупреждение
			//	}
			//	continue;
			//}
			//// Вывод информации об отправленном кадре
			//std::cout << "- [ OK ] Sent frame: " << frame_counter
			//	<< " Size: " << frame.cols << "x" << frame.rows
			//	<< " Channels: " << frame.channels()
			//	<< " Serialized size: " << serialized.size() << " bytes" << std::endl;


			if (frame_counter) {
				std::string filename = (temp_dir / ("frame_" + std::to_string(frame_counter) + ".jpg")).string();
				cv::imwrite(filename, frame);
				//std::cout << "  Saved: " << filename << std::endl; // Вывод сообщения о сохранении
			}

			//cv::imshow("Capturer - Camera Feed", frame);

			std::cout << "- [ OK ] Captured frame: " << frame_counter++
				<< " Size: " << frame.cols << "x" << frame.rows
				<< " Channels: " << frame.channels() << std::endl;

			if (cv::waitKey(1) == 27) break;
		}

		cap.release(); // Освобождение ресурсов камеры
		cv::destroyAllWindows(); // Закрытие всех окон OpenCV
	}
};

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

}