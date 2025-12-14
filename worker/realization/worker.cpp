#include <iostream>
#include <vector>
#include <zmq.hpp>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <thread>
#include <atomic>
#include "ImageStructure.hpp"

class SimpleWorker {
private:
    zmq::context_t context;
    zmq::socket_t pull_socket;
    std::string worker_id;
    std::filesystem::path temp_dir;
    std::atomic<size_t> frame_counter{0};
    std::atomic<bool> should_exit{false};

public:
    SimpleWorker() : context(1), pull_socket(context, ZMQ_PULL) {
        // Подключение к Capturer
        std::string connect_address = "tcp://localhost:5555";
        
        try {
            pull_socket.connect(connect_address);
            pull_socket.set(zmq::sockopt::rcvtimeo, 1000);  // Таймаут 1 секунда
            std::cout << "Worker connected to: " << connect_address << std::endl;
        }
        catch (const zmq::error_t& e) {
            std::cout << "Failed to connect: " << e.what() << std::endl;
            throw std::runtime_error("Failed to connect to Capturer");
        }

        // Создаем ID воркера
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        std::stringstream ss;
        ss << "simple_worker_" << ms;
        worker_id = ss.str();

        // Создаем временную папку
        temp_dir = std::filesystem::current_path() / worker_id;
        std::filesystem::create_directories(temp_dir);

        std::cout << "Worker ID: " << worker_id << std::endl;
        std::cout << "Temp directory: " << temp_dir << std::endl;
        std::cout << "======================================" << std::endl;
    }

private:
    void save_image(const cv::Mat& image, size_t frame_id) {
        std::stringstream filename;
        filename << "frame_" << std::setfill('0') << std::setw(6) << frame_id << ".jpg";
        std::filesystem::path filepath = temp_dir / filename.str();
        
        try {
            std::vector<int> compression_params = {cv::IMWRITE_JPEG_QUALITY, 80};
            if (cv::imwrite(filepath.string(), image, compression_params)) {
                //std::cout << "- Saved frame: " << frame_id << " to " << filename.str() << std::endl;
            } else {
                std::cout << "- Failed to save frame: " << frame_id << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "- Error saving frame: " << e.what() << std::endl;
        }
    }

public:
    void run() {
        std::cout << "=== Simple Worker Started ===" << std::endl;
        std::cout << "Receiving and saving frames..." << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;
        std::cout << "======================================" << std::endl;

        size_t frames_received = 0;
        auto start_time = std::chrono::steady_clock::now();

        while (!should_exit) {
            try {
                zmq::message_t msg;
                
                // Получаем сообщение
                if (!pull_socket.recv(msg, zmq::recv_flags::dontwait)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                frame_counter++;
                size_t current_frame = frame_counter.load();
                
                // Десериализация
                cv::Mat received_image;
                ImageStructure img_struct(received_image);
                std::string serialized_data(static_cast<char*>(msg.data()), msg.size());
                img_struct.deserialize(serialized_data);
                
                size_t frame_id = img_struct.id;
                
                std::cout << "\n--- Frame " << frame_id << " (#" << current_frame << ") ---" << std::endl;
                std::cout << "Size: " << msg.size() << " bytes, " 
                          << received_image.cols << "x" << received_image.rows << std::endl;
                
                if (received_image.empty()) {
                    std::cout << "- Empty image, skipping..." << std::endl;
                    continue;
                }
                
                // Сохраняем изображение
                save_image(received_image, frame_id);
                frames_received++;
                
                // Статистика каждые 50 кадров
                if (frames_received % 50 == 0) {
                    auto current_time = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        current_time - start_time).count();
                    if (elapsed > 0) {
                        double fps = static_cast<double>(frames_received) / elapsed;
                        std::cout << "\n=== Statistics ===" << std::endl;
                        std::cout << "Frames received: " << frames_received << std::endl;
                        std::cout << "Average FPS: " << fps << std::endl;
                        std::cout << "=====================================" << std::endl;
                    }
                }
                
            } catch (const zmq::error_t& e) {
                if (e.num() == ETERM) {
                    std::cout << "ZMQ context terminated" << std::endl;
                    break;
                }
                std::cout << "- ZMQ error: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } catch (const std::exception& e) {
                std::cout << "- Error: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }      
    }
};

int main() {
    // Устанавливаем локаль для корректного вывода
    std::locale::global(std::locale("C"));
    
    try {
        SimpleWorker worker;
        worker.run();
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n======================================" << std::endl;
        std::cout << "FATAL ERROR: " << e.what() << std::endl;
        std::cout << "======================================" << std::endl;
        return -1;
    }
}