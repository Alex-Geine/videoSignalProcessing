#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <string>

// Структура для хранения кадра с индексом
struct FrameWithIndex
{
    cv::Mat frame; // Сам кадр
    int index;     // Индекс кадра
};

// Перечисление для частей буфера
enum class BufferPart
{
    FIRST = 0,
    SECOND = 1,
    THIRD = 2
};

class PostProcessor
{
public:
    // Конструктор с директорией для сохранения
    PostProcessor(int bufferSize = 90, int timeoutMs = 5000, const std::string &outputDir = "./videos");

    // Деструктор
    ~PostProcessor();

    // Добавление кадра в буфер
    void addFrame(const cv::Mat &frame, int index);

    // Запуск постобработчика
    void start();

    // Остановка постобработчика
    void stop();

    // Принудительное сохранение всех кадров
    void flushAll();

    // Установка директории для сохранения видео
    void setOutputDirectory(const std::string &dir);

private:
    // Размер буфера
    int maxFrames;

    // Текущий индекс в буфере
    int currentFrameIndex;

    // Флаг первого запуска
    bool firstRun;

    // Текущая заполняемая часть буфера
    BufferPart currentlyFilling;

    // Размер одной части буфера
    int bufferPartSize;

    // Буфер кадров
    std::vector<FrameWithIndex> frameBuffer;

    // Мьютекс для синхронизации доступа к буферу
    std::mutex bufferMutex;

    // Флаг работы постобработчика
    bool isRunning;

    // Поток проверки таймаута
    std::thread timeoutThread;

    // Время последнего полученного кадра
    std::chrono::steady_clock::time_point lastFrameTime;

    // Длительность таймаута в миллисекундах
    std::chrono::milliseconds timeoutDuration;

    // Директория для сохранения видео
    std::string outputDirectory;

    // Инициализация буфера черными кадрами
    void initializeBuffer();

    // Создание черного кадра
    cv::Mat createBlackFrame(int width, int height);

    // Проверка необходимости сохранения части буфера
    void checkAndSaveIfNeeded();

    // Сохранение части буфера в видео
    void savePartToVideo(BufferPart partToSave);

    // Сохранение кадров в видеофайл
    void saveFramesToVideo(const std::vector<FrameWithIndex> &frames, const std::string &filename);

    // Сброс части буфера к черным кадрам
    void resetBufferPart(BufferPart part);

    // Поток проверки таймаута
    void timeoutChecker();
};

#endif // POSTPROCESSOR_H