#include <iostream>
#include "postProcessor.h"

// Файл реализации PostProcessor.cpp
// Содержит реализацию методов класса PostProcessor

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem> // Добавляем для работы с файловой системой
#include "utils.h"

namespace fs = std::filesystem;

// Конструктор с параметрами по умолчанию
PostProcessor::PostProcessor(int bufferSize, int timeoutMs, const std::string &outputDir)
    : maxFrames(bufferSize),
      currentFrameIndex(0),
      firstRun(true),
      currentlyFilling(BufferPart::FIRST),
      isRunning(false),
      timeoutDuration(timeoutMs),
      outputDirectory(outputDir)
{
    // Создаем директорию для сохранения видео, если она не существует
    if (!outputDirectory.empty())
    {
        try
        {
            fs::create_directories(outputDirectory);
            std::cout << "Директория для сохранения видео: " << outputDirectory << std::endl;
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "Ошибка при создании директории " << outputDirectory
                      << ": " << e.what() << std::endl;
            // Используем текущую директорию как запасной вариант
            outputDirectory = ".";
        }
    }
    else
    {
        outputDirectory = "."; // Сохраняем в текущую директорию
    }

    // Проверка корректности размера буфера
    if (maxFrames % 3 != 0)
    {
        // Если размер не кратен 3, увеличиваем до ближайшего кратного 3
        maxFrames = ((maxFrames / 3) + 1) * 3;
        std::cout << "Размер буфера скорректирован до " << maxFrames
                  << " (должен быть кратен 3)" << std::endl;
    }

    // Вычисление размера одной части буфера
    bufferPartSize = maxFrames / 3;

    // Инициализация буфера
    frameBuffer.resize(maxFrames);
    initializeBuffer();

    // Инициализация времени последнего кадра
    lastFrameTime = std::chrono::steady_clock::now();

    std::cout << "PostProcessor инициализирован с размером буфера: "
              << maxFrames << " кадров" << std::endl;
    std::cout << "Размер одной части: " << bufferPartSize << " кадров" << std::endl;
}

// Деструктор
PostProcessor::~PostProcessor()
{
    stop(); // Останавливаем постобработчик перед удалением
}

// Инициализация буфера черными кадрами
void PostProcessor::initializeBuffer()
{
    // Проходим по всем элементам буфера
    for (int i = 0; i < maxFrames; i++)
    {
        // Создаем черный кадр с минимальными размерами
        // Реальные размеры будут установлены при получении первого кадра
        frameBuffer[i].frame = cv::Mat::zeros(480, 640, CV_8UC3);
        frameBuffer[i].index = -1; // Индекс -1 означает пустой кадр
    }
    std::cout << "Буфер инициализирован черными кадрами" << std::endl;
}

// Создание черного кадра заданного размера
cv::Mat PostProcessor::createBlackFrame(int width, int height)
{
    // Создаем матрицу заданного размера, заполненную нулями (черный цвет)
    // CV_8UC3 означает: 8 бит на канал, 3 канала (BGR)
    return cv::Mat::zeros(height, width, CV_8UC3);
}

// Добавление нового кадра в буфер
void PostProcessor::addFrame(const cv::Mat &frame, int index)
{
    std::lock_guard<std::mutex> lock(bufferMutex); // Блокируем мьютекс для безопасного доступа

    // Проверяем, что кадр не пустой
    if (frame.empty())
    {
        std::cerr << "Предупреждение: получен пустой кадр с индексом " << index << std::endl;
        return;
    }

    // Обновляем время получения последнего кадра
    lastFrameTime = std::chrono::steady_clock::now();

    // Если это первый кадр, проверяем размеры черных кадров
    if (currentFrameIndex == 0)
    {
        // Обновляем все черные кадры в буфере до правильного размера
        for (int i = 0; i < maxFrames; i++)
        {
            if (frameBuffer[i].frame.size() != frame.size())
            {
                frameBuffer[i].frame = createBlackFrame(frame.cols, frame.rows);
            }
        }
    }
    currentFrameIndex = index % maxFrames;
    // Сохраняем кадр в буфер
    if (currentFrameIndex < maxFrames)
    {
        frame.copyTo(frameBuffer[currentFrameIndex].frame); // Копируем кадр
        frameBuffer[currentFrameIndex].index = index;       // Сохраняем индекс
        currentFrameIndex++;                                // Увеличиваем индекс

        std::cout << "Кадр " << index << " добавлен в буфер на позицию "
                  << currentFrameIndex - 1 << std::endl;

        // Проверяем, нужно ли сохранять часть буфера
        checkAndSaveIfNeeded();
    }
    else
    {
        std::cerr << "Ошибка: буфер переполнен" << std::endl;
    }
}

// Проверка необходимости сохранения части буфера
void PostProcessor::checkAndSaveIfNeeded()
{
    // Определяем, какая часть буфера сейчас заполняется
    if (firstRun)
    {
        // При первом прогоне ждем заполнения третьей части
        if (currentFrameIndex >= bufferPartSize * 2)
        {
            std::cout << "Первые две части заполнены, начинается заполнение третьей" << std::endl;
            currentlyFilling = BufferPart::THIRD;
            savePartToVideo(BufferPart::FIRST);
            firstRun = false;
        }
    }
    else
    {
        int currentPart = currentFrameIndex / bufferPartSize;
        std::cout << "currentPart " << currentPart << "\n";
        std::cout << "currentlyFilling " << (int)currentlyFilling << "\n";
        std::cout << "first cond " << (currentPart == 1 && (int)currentlyFilling == 0) << "\n";
        std::cout << "second cond " << (currentPart == 2 && (int)currentlyFilling == 1) << "\n";
        std::cout << "third cond " << (currentPart == 0 && (int)currentlyFilling == 2) << "\n";
        std::cout << "all cond " << ((currentPart == 1 && (int)currentlyFilling == 0) || (currentPart == 2 && (int)currentlyFilling == 1) || (currentPart == 0 && (int)currentlyFilling == 2)) << "\n";
        // При последующих прогонах проверяем границы частей
        if (
            (currentPart == 1 && (int)currentlyFilling == 0) ||
            (currentPart == 2 && (int)currentlyFilling == 1) ||
            (currentPart == 0 && (int)currentlyFilling == 2))
        {
            switch (currentPart)
            {
            case 0:
                currentlyFilling = BufferPart::FIRST;
                // Сохраняем третью часть
                savePartToVideo(BufferPart::SECOND);
                break;
            case 1:
                currentlyFilling = BufferPart::SECOND;
                // Сохраняем первую часть
                savePartToVideo(BufferPart::THIRD);
                break;
            case 2:
                currentlyFilling = BufferPart::THIRD;
                // Сохраняем вторую часть
                savePartToVideo(BufferPart::FIRST);
                // Сбрасываем индекс для циклической записи
                currentFrameIndex = 0;
                break;
            }
        }
    }
}

// Сохранение части буфера в видео
void PostProcessor::savePartToVideo(BufferPart partToSave)
{
    std::cout << "start save\n";

    // Определяем индексы начала и конца сохраняемой части
    int startIdx, endIdx;
    switch (partToSave)
    {
    case BufferPart::FIRST:
        startIdx = 0;
        endIdx = bufferPartSize - 1;
        break;
    case BufferPart::SECOND:
        startIdx = bufferPartSize;
        endIdx = bufferPartSize * 2 - 1;
        break;
    case BufferPart::THIRD:
        startIdx = bufferPartSize * 2;
        endIdx = maxFrames - 1;
        break;
    }

    // Проверяем, есть ли кадры для сохранения в этой части
    bool hasFrames = false;
    for (int i = startIdx; i <= endIdx; i++)
    {
        if (frameBuffer[i].index != -1)
        {
            hasFrames = true;
            break;
        }
    }

    std::cout << "exist frames: " << (hasFrames ? "true" : "false") << std::endl;

    if (!hasFrames)
    {
        std::cout << "В части " << static_cast<int>(partToSave)
                  << " нет кадров для сохранения" << std::endl;
        return;
    }

    // Создаем копию кадров для сохранения
    std::vector<FrameWithIndex> framesToSave;
    std::string currentOutputDir;

    {
        // std::lock_guard<std::mutex> lock(bufferMutex);
        std::cout << "is mutex\n";
        // Копируем кадры из указанной части буфера
        for (int i = startIdx; i <= endIdx; i++)
        {
            FrameWithIndex frameCopy;
            frameBuffer[i].frame.copyTo(frameCopy.frame);
            frameCopy.index = frameBuffer[i].index;
            framesToSave.push_back(frameCopy);
        }

        // Сохраняем текущую директорию (захватим ее в лямбде)
        currentOutputDir = outputDirectory;

        // Сбрасываем сохраненные кадры в буфере к черным кадрам
        resetBufferPart(partToSave);
    }

    std::cout << "created copying frames: " << framesToSave.size() << " frames\n";

    // Проверяем, что есть кадры для сохранения
    if (framesToSave.empty())
    {
        std::cout << "Нет кадров для сохранения после копирования\n";
        return;
    }

    // Создаем локальную копию выходной директории
    std::string localOutputDir = currentOutputDir;

    // Запускаем сохранение в отдельном потоке
    std::thread saveThread([this, framesToSave, partToSave, localOutputDir]() mutable
                           {
        try
        {
            std::cout << "start thread for part " << static_cast<int>(partToSave) << "\n";
            
            // Генерируем имя файла с временной меткой
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;

// Безопасное получение локального времени
#ifdef _WIN32
                localtime_s(&tm_now, &time_t_now);
#else
                localtime_r(&time_t_now, &tm_now);
#endif
            
            std::stringstream filename;
            filename << "video_part_" << static_cast<int>(partToSave) << "_"
                     << std::put_time(&tm_now, "%Y%m%d_%H%M%S") << ".avi";
            
            std::cout << "output directory: " << localOutputDir << "\n";
            
            // Формируем полный путь для сохранения
            std::string fullPath;
            if (!localOutputDir.empty() && localOutputDir != ".")
            {
                // Создаем директорию, если она не существует
                try
                {
                    if (!fs::exists(localOutputDir))
                    {
                        std::cout << "Creating directory: " << localOutputDir << std::endl;
                        fs::create_directories(localOutputDir);
                    }
                    fullPath = localOutputDir + "/" + filename.str();
                }
                catch (const fs::filesystem_error& e)
                {
                    std::cerr << "Error creating directory: " << e.what() << std::endl;
                    fullPath = filename.str(); // Сохраняем в текущую директорию
                }
            }
            else
            {
                fullPath = filename.str();
            }
            
            std::cout << "fullPath: " << fullPath << "\n";
            
            // Сохраняем кадры в видео
            saveFramesToVideo(framesToSave, fullPath);
            
            std::cout << "Thread finished for part " << static_cast<int>(partToSave) << "\n";
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception in save thread: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown exception in save thread" << std::endl;
        } });

    // Отсоединяем поток, чтобы он работал независимо
    saveThread.detach();

    std::cout << "Thread detached for part " << static_cast<int>(partToSave)
              << ". Frames to save: " << framesToSave.size() << std::endl;

    std::cout << "Запущено сохранение части " << static_cast<int>(partToSave)
              << " в файл: " << outputDirectory << std::endl;
}

// Сохранение кадров в видеофайл
void PostProcessor::saveFramesToVideo(const std::vector<FrameWithIndex> &frames, const std::string &filename)
{
    if (frames.empty())
    {
        std::cerr << "Нет кадров для сохранения в файл " << filename << std::endl;
        return;
    }

    try
    {
        std::cout << "saveFramesToVideo\n";
        // Получаем размеры первого кадра
        cv::Size frameSize = frames[0].frame.size();

        // Создаем объект VideoWriter
        // FOURCC 'XVID' - кодек для AVI файлов
        // 30.0 - FPS (кадров в секунду)
        cv::VideoWriter videoWriter(filename,
                                    cv::VideoWriter::fourcc('X', 'V', 'I', 'D'),
                                    30.0,
                                    frameSize);

        if (!videoWriter.isOpened())
        {
            std::cerr << "Ошибка: не удалось создать видеофайл " << filename << std::endl;

            // Пробуем альтернативный кодек
            std::cerr << "Попытка использовать кодек MJPG..." << std::endl;
            videoWriter.open(filename,
                             cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                             30.0,
                             frameSize);

            if (!videoWriter.isOpened())
            {
                std::cerr << "Ошибка: не удалось создать видеофайл с кодеком MJPG" << std::endl;
                return;
            }
        }

        // Записываем каждый кадр в видео
        for (const auto &frameWithIndex : frames)
        {
            videoWriter.write(frameWithIndex.frame);
        }

        // Закрываем видеофайл
        videoWriter.release();

        std::cout << "Видео сохранено: " << filename
                  << " (кадров: " << frames.size() << ")" << std::endl;
    }
    catch (const cv::Exception &e)
    {
        std::cerr << "Ошибка OpenCV при сохранении видео: " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка при сохранении видео: " << e.what() << std::endl;
    }
}

// Сброс части буфера к черным кадрам
void PostProcessor::resetBufferPart(BufferPart part)
{
    int startIdx, endIdx;

    // Определяем границы части буфера
    switch (part)
    {
    case BufferPart::FIRST:
        startIdx = 0;
        endIdx = bufferPartSize - 1;
        break;
    case BufferPart::SECOND:
        startIdx = bufferPartSize;
        endIdx = bufferPartSize * 2 - 1;
        break;
    case BufferPart::THIRD:
        startIdx = bufferPartSize * 2;
        endIdx = maxFrames - 1;
        break;
    }

    // Сбрасываем кадры в указанном диапазоне
    for (int i = startIdx; i <= endIdx; i++)
    {
        if (!frameBuffer[i].frame.empty())
        {
            // Сохраняем размеры для создания черного кадра
            int width = frameBuffer[i].frame.cols;
            int height = frameBuffer[i].frame.rows;
            frameBuffer[i].frame = createBlackFrame(width, height);
        }
        frameBuffer[i].index = -1; // Отмечаем как пустой
    }

    std::cout << "Часть буфера " << static_cast<int>(part)
              << " сброшена к черным кадрам" << std::endl;
}

// Принудительное сохранение всех кадров
void PostProcessor::flushAll()
{
    std::lock_guard<std::mutex> lock(bufferMutex);

    // Сохраняем все заполненные части буфера
    if (currentFrameIndex > 0)
    {
        // Определяем, какие части нужно сохранить
        if (firstRun)
        {
            // При первом прогоне сохраняем все заполненные кадры
            int filledParts = (currentFrameIndex - 1) / bufferPartSize + 1;
            for (int part = 0; part < filledParts; part++)
            {
                savePartToVideo(static_cast<BufferPart>(part));
            }
        }
        else
        {
            // Сохраняем все три части
            savePartToVideo(BufferPart::FIRST);
            savePartToVideo(BufferPart::SECOND);
            savePartToVideo(BufferPart::THIRD);
        }
    }

    std::cout << "Все кадры сохранены в директорию: " << outputDirectory << std::endl;
}

// Поток проверки таймаута
void PostProcessor::timeoutChecker()
{
    while (isRunning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Проверяем каждые 100 мс

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastFrameTime);

        // Если время с последнего кадра превысило таймаут
        if (elapsed > timeoutDuration)
        {
            std::cout << "Таймаут истек, сохраняем оставшиеся кадры..." << std::endl;
            flushAll();
            lastFrameTime = now; // Сбрасываем таймер
        }
    }
}

// Запуск постобработчика
void PostProcessor::start()
{
    if (!isRunning)
    {
        isRunning = true;
        // Запускаем поток проверки таймаута
        timeoutThread = std::thread(&PostProcessor::timeoutChecker, this);
        std::cout << "PostProcessor запущен. Видео сохраняются в: " << outputDirectory << std::endl;
    }
}

// Остановка постобработчика
void PostProcessor::stop()
{
    if (isRunning)
    {
        isRunning = false;
        // Ожидаем завершения потока таймаута
        if (timeoutThread.joinable())
        {
            timeoutThread.join();
        }
        // Сохраняем все оставшиеся кадры
        flushAll();
        std::cout << "PostProcessor остановлен" << std::endl;
    }
}

// Метод для изменения директории сохранения
void PostProcessor::setOutputDirectory(const std::string &dir)
{
    if (!dir.empty())
    {
        try
        {
            fs::create_directories(dir);
            outputDirectory = dir;
            std::cout << "Директория для сохранения видео изменена на: " << outputDirectory << std::endl;
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "Ошибка при создании директории " << dir
                      << ": " << e.what() << std::endl;
        }
    }
}

int main()
{
    std::cout << "real" << std::endl;
    Utils postprocessor;
    postprocessor.loadConfig();

    std::string ip = postprocessor.getConfig("postprocessor.ip");
    int port = std::stoi(postprocessor.getConfig("postprocessor.port"));
    std::string output_dir = postprocessor.getConfig("postprocessor.output_dir");
    std::string proc_prefix = postprocessor.getConfig("postprocessor.processed_prefix");
    std::string bare_prefix = postprocessor.getConfig("postprocessor.bare_prefix");
    int maxFrames = std::stoi(postprocessor.getConfig("postprocessor.max_frames"));
    int timeoutDuration = std::stoi(postprocessor.getConfig("postprocessor.timeout_duration"));

    static int image_counter = 1;

    if (!postprocessor.initializeServer(ip, port))
    {
        return -1;
    }

    // Создаем экземпляр PostProcessor с указанием директории для сохранения видео
    // Буфер на 90 кадров (30 кадров в каждой из 3 частей)
    // Таймаут 5 секунд
    // Директория для сохранения видео берется из конфигурации
    PostProcessor videoProcessor(maxFrames, timeoutDuration, output_dir);
    videoProcessor.start(); // Запускаем постобработчик

    std::cout << "PostProcessor started. Waiting for server..." << std::endl;

    while (true)
    {
        std::cout << "\nWaiting for server..." << std::endl;

        // Ждем запрос от сервера
        std::string request = postprocessor.receiveMessage();
        if (request == "READY")
        {
            std::cout << "Server is ready. Receiving images..." << std::endl;

            // Отправляем подтверждение, что готовы получать изображения
            postprocessor.sendMessage("SEND_FIRST_IMAGE");

            // Получаем первое изображение (обработанное)
            cv::Mat processed_image = postprocessor.receiveImage();
            if (!processed_image.empty())
            {
                postprocessor.setCurrentImage(processed_image);
                std::string proc_filename = output_dir + proc_prefix + std::to_string(image_counter) + ".bmp";
                // postprocessor.saveImage(proc_filename);
                // std::cout << "Saved processed image: " << proc_filename << std::endl;

                // Добавляем кадр в PostProcessor
                videoProcessor.addFrame(processed_image, image_counter); // Тут вместо image_counter передаем номер кадра

                // Подтверждаем получение первого изображения
                postprocessor.sendMessage("SEND_SECOND_IMAGE");

                // Получаем второе изображение (оригинальное)
                cv::Mat original_image = postprocessor.receiveImage();
                if (!original_image.empty())
                {
                    postprocessor.setCurrentImage(original_image);
                    std::string bare_filename = output_dir + bare_prefix + std::to_string(image_counter) + ".bmp";
                    // postprocessor.saveImage(bare_filename);
                    // std::cout << "Saved original image: " << bare_filename << std::endl;

                    // Подтверждаем завершение
                    postprocessor.sendMessage("DONE");

                    std::cout << "Postprocessing completed. Total: " << image_counter << std::endl;
                    image_counter += image_counter % 2 == 0 ? 20 : 1;
                }
            }
        }
    }

    videoProcessor.stop(); // Останавливаем постобработчик перед выходом

    return 0;
}