#pragma once

#include <cstddef>
#include <cstring>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <zmq.hpp>

struct ImageStructure
{
    cv::Mat image;  // ← Теперь ХРАНИТ данные
    size_t id = 0;

    // Конструктор из существующего изображения
    ImageStructure(const cv::Mat& m, size_t frame_id = 0)
        : id(frame_id)
    {
        if (m.empty()) {
            image = cv::Mat();
            return;
        }

        // Приводим к единому формату CV_8UC3
        if (m.type() == CV_8UC3) {
            image = m.clone(); // копируем, чтобы избежать внешней зависимости
        } else {
            cv::Mat temp;
            m.convertTo(temp, CV_8UC3);
            image = temp;
        }
    }

    // Конструктор по умолчанию (для десериализации)
    ImageStructure() = default;

    std::vector<uint8_t> serialize() const
    {
        if (image.empty()) {
            return {};
        }

        size_t rows = image.rows;
        size_t cols = image.cols;
        size_t size = image.total() * image.elemSize();

        std::vector<uint8_t> buffer;
        buffer.reserve(sizeof(id) + 3 * sizeof(size_t) + size);

        // Помощник для добавления POD-данных
        auto write_pod = [&buffer](const auto& value) {
            const char* ptr = reinterpret_cast<const char*>(&value);
            buffer.insert(buffer.end(), ptr, ptr + sizeof(value));
        };

        write_pod(id);
        write_pod(size);
        write_pod(rows);
        write_pod(cols);
        buffer.insert(buffer.end(), image.data, image.data + size);

        return buffer;
    }

    bool deserialize(const std::vector<uint8_t>& data)
    {
        if (data.size() < sizeof(id) + 3 * sizeof(size_t)) {
            return false;
        }

        size_t offset = 0;
        auto read_pod = [&data, &offset](auto& value) -> bool {
            size_t size = sizeof(value);
            if (offset + size > data.size()) return false;
            std::memcpy(&value, data.data() + offset, size);
            offset += size;
            return true;
        };

        if (!read_pod(id)) return false;
        size_t size, rows, cols;
        if (!read_pod(size) || !read_pod(rows) || !read_pod(cols)) return false;

        if (offset + size != data.size()) return false;

        image = cv::Mat(rows, cols, CV_8UC3);
        std::memcpy(image.data, data.data() + offset, size);
        return true;
    }

    bool deserialize(const zmq::message_t& msg)
    {
        std::vector<uint8_t> data(static_cast<const uint8_t*>(msg.data()), 
                                  static_cast<const uint8_t*>(msg.data()) + msg.size());
        return deserialize(data);
    }

    const cv::Mat& getImage() const { return image; }
    size_t getFrameId() const { return id; }
};