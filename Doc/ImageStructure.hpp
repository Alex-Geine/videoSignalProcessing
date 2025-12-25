/**
 * @file ImageStructure.hpp
 * @brief Сериализация / десериализация изображений для распределенной системы обработки видео
 *
 * Этот заголовочный файл определяет структуру 'ImageStructure' - универсальный контейнер
 * для передачи видеокадров между компонентами системы РОВс (Распределенной Обработки
 * Видео Сигнала) через сеть ZeroMQ.
 *
 * Использование:
 * - Сервер: сериализация кадров с камеры -> ZeroMQ
 * - Worker: десериализация -> обработка -> сериализация -> ZeroMQ
 * - PostProcessor: десериализация -> буферизация -> сохранение
 *
 * Требования:
 * - OpenCV 4.0
 * - ZeroMQ 4.3.5
 * - C++ 17 или новее
 */
#pragma once

#include <cstddef>
#include <cstring>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <zmq.hpp>

 /**
  * @struct ImageStructure
  * @brief Универсальный контейнер для сериализации видеокадров
  *
  * Инкапсулирует изображение OpenCV вместе с данными (ID кадра)
  * и предоставляет методы для бинарной сериализации/десериализации,
  * оптимизированные для сетевой передачи через ZeroMQ.
  */

struct ImageStructure
{
    cv::Mat image;  ///< Хранит данные изображения
    size_t id = 0;  ///< Идентификатор кадра (порядковый номер)

    /**
     * @brief Конструктор из существующего изображения OpenCV
     *
     * Создает глубокую копию изображения с автоматическим приведением
     * к единому формату CV_8UC3 для гарантии совместимости.
     *
     * @param m Входное изображение OpenCV (любого формата)
     * @param frame_id Идентификатор кадра (по умолчанию 0)
     *
     * @note Если изображение пустое, создается пустой cv::Mat
     */
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
        }
        else {
            cv::Mat temp;
            m.convertTo(temp, CV_8UC3);
            image = temp;
        }
    }

    /**
     * @brief Конструктор по умолчанию (для десериализации)
     *
     * Создает пустую структуру.
     */
    ImageStructure() = default;
    /**
     * @brief Сериализует структуру в бинарный формат
     *
     * Преобразует изображение и данные в компактный
     * бинарный формат, пригодный для сетевой передачи.
     *
     * @return Вектор байтов, содержащий сериализованные данные
     */
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

    /**
     * @brief Десериализует структуру из бинарных данных
     *
     * Восстанавливает изображение и данные из сериализованного
     * формата с проверкой целостности данных.
     *
     * @param data Вектор байтов, содержащий сериализованные данные
     * @return true если десериализация успешна, false в противном случае
     *
     * @note Проверяет размер данных и соответствие заявленных размеров
     * @warning Требует точного соответствия формату serialize()
     */
    bool deserialize(const std::vector<uint8_t>& data)
    {
        if (data.size() < sizeof(id) + 3 * sizeof(size_t)) {
            return false;
        }

        size_t offset = 0;
        auto read_pod = [&data, &offset](auto& value)  ->  bool {
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
    /**
     * @brief Десериализует структуру из сообщения ZeroMQ
     *
     * Удобная обертка для десериализации непосредственно из
     * сообщения ZeroMQ без промежуточного копирования.
     *
     * @param msg Сообщение ZeroMQ, содержащее сериализованные данные
     * @return true если десериализация успешна, false в противном случае
     *
     * @note Фактически копирует данные из сообщения во временный буфер
     */
    bool deserialize(const zmq::message_t& msg)
    {
        std::vector<uint8_t> data(static_cast<const uint8_t*>(msg.data()),
            static_cast<const uint8_t*>(msg.data()) + msg.size());
        return deserialize(data);
    }

    /**
     * @brief Возвращает ссылку на изображение
     *
     * @return Константная ссылка на cv::Mat с изображением
     */
    const cv::Mat& getImage() const { return image; }

    /**
     * @brief Возвращает идентификатор кадра
     *
     * @return ID кадра (порядковый номер)
     */
    size_t getFrameId() const { return id; }
};