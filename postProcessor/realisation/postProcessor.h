#ifndef _POST_PROCESSOR_H_
#define _POST_PROCESSOR_H_

// Заголовочный файл PostProcessor.h
// Содержит объявление класса PostProcessor для обработки и сохранения последовательностей кадров

#include <opencv2/opencv.hpp> // Библиотека для работы с изображениями и видео
#include <vector>             // Для использования динамических массивов
#include <thread>             // Для работы с потоками
#include <mutex>              // Для синхронизации доступа к общим ресурсам
#include <atomic>             // Для атомарных операций
#include <condition_variable> // Для условных переменных
#include <chrono>             // Для работы со временем
#include <string>             // Для работы со строками

class PostProcessor
{
private:
    // Структура для хранения кадра с индексом
    struct FrameWithIndex
    {
        cv::Mat frame; // Сам кадр (изображение)
        int index;     // Индекс кадра в последовательности
    };

    // Основные параметры
    std::vector<FrameWithIndex> frameBuffer; // Основной буфер кадров
    int maxFrames;                           // Максимальное число кадров в буфере
    int bufferPartSize;                      // Размер одной части буфера
    int currentFrameIndex;                   // Текущий индекс для записи кадров
    bool firstRun;                           // Флаг первого прогона

    // Состояние заполнения частей буфера
    enum class BufferPart
    {
        FIRST,
        SECOND,
        THIRD
    };
    BufferPart currentlyFilling; // Какая часть буфера сейчас заполняется

    // Механизмы синхронизации
    std::mutex bufferMutex;      // Мьютекс для защиты доступа к буферу
    std::atomic<bool> isRunning; // Атомарный флаг работы постобработчика

    // Механизм таймера для принудительного сохранения
    std::chrono::steady_clock::time_point lastFrameTime; // Время получения последнего кадра
    std::chrono::milliseconds timeoutDuration;           // Таймаут для принудительного сохранения
    std::thread timeoutThread;                           // Поток для проверки таймаута

    // Вспомогательные методы
    void initializeBuffer();                                                                        // Инициализация буфера черными кадрами
    cv::Mat createBlackFrame(int width, int height);                                                // Создание черного кадра
    void savePartToVideo(BufferPart partToSave);                                                    // Сохранение части буфера в видео
    void saveFramesToVideo(const std::vector<FrameWithIndex> &frames, const std::string &filename); // Сохранение кадров в видеофайл
    void resetBufferPart(BufferPart part);                                                          // Сброс части буфера к черным кадрам
    void checkAndSaveIfNeeded();                                                                    // Проверка необходимости сохранения
    void timeoutChecker();                                                                          // Поток проверки таймаута

public:
    // Конструктор и деструктор
    PostProcessor(int bufferSize = 90, int timeoutMs = 5000); // Конструктор с параметрами
    ~PostProcessor();                                         // Деструктор для очистки ресурсов

    // Основные методы
    void addFrame(const cv::Mat &frame, int index); // Добавление нового кадра в буфер
    void flushAll();                                // Принудительное сохранение всех кадров

    // Методы для управления
    void start(); // Запуск постобработчика
    void stop();  // Остановка постобработчика
};

#endif // _POST_PROCESSOR_H_