#include <iostream>
#include "postProcessor.h"

// Файл реализации PostProcessor.cpp
// Содержит реализацию методов класса PostProcessor

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "utils.h"

// Конструктор с параметрами по умолчанию
PostProcessor::PostProcessor(int bufferSize, int timeoutMs)
    : maxFrames(bufferSize),
      currentFrameIndex(0),
      firstRun(true),
      currentlyFilling(BufferPart::FIRST),
      isRunning(false),
      timeoutDuration(timeoutMs)
{

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
void PostProcessor::PostProcessor::addFrame(const cv::Mat &frame, int index)
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
            firstRun = false;
        }
    }
    else
    {
        // При последующих прогонах проверяем границы частей
        if (currentFrameIndex % bufferPartSize == 0)
        {
            int currentPart = currentFrameIndex / bufferPartSize;
            switch (currentPart)
            {
            case 1:
                currentlyFilling = BufferPart::FIRST;
                // Сохраняем третью часть
                savePartToVideo(BufferPart::THIRD);
                break;
            case 2:
                currentlyFilling = BufferPart::SECOND;
                // Сохраняем первую часть
                savePartToVideo(BufferPart::FIRST);
                break;
            case 3:
                currentlyFilling = BufferPart::THIRD;
                // Сохраняем вторую часть
                savePartToVideo(BufferPart::SECOND);
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

    if (!hasFrames)
    {
        std::cout << "В части " << static_cast<int>(partToSave)
                  << " нет кадров для сохранения" << std::endl;
        return;
    }

    // Создаем копию кадров для сохранения
    std::vector<FrameWithIndex> framesToSave;
    {
        std::lock_guard<std::mutex> lock(bufferMutex);

        // Копируем кадры из указанной части буфера
        for (int i = startIdx; i <= endIdx; i++)
        {
            if (frameBuffer[i].index != -1)
            {
                FrameWithIndex frameCopy;
                frameBuffer[i].frame.copyTo(frameCopy.frame);
                frameCopy.index = frameBuffer[i].index;
                framesToSave.push_back(frameCopy);
            }
        }

        // Сбрасываем сохраненные кадры в буфере к черным кадрам
        resetBufferPart(partToSave);
    }

    // Запускаем сохранение в отдельном потоке
    std::thread saveThread([this, framesToSave, partToSave]()
                           {
        // Генерируем имя файла с временной меткой
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);
        
        std::stringstream filename;
        filename << "video_part_" << static_cast<int>(partToSave) << "_"
                 << std::put_time(&tm_now, "%Y%m%d_%H%M%S") << ".avi";
        
        // Сохраняем кадры в видео
        saveFramesToVideo(framesToSave, filename.str()); });

    // Отсоединяем поток, чтобы он работал независимо
    saveThread.detach();

    std::cout << "Запущено сохранение части " << static_cast<int>(partToSave)
              << " в отдельном потоке" << std::endl;
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
            return;
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

    std::cout << "Все кадры сохранены" << std::endl;
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
        std::cout << "PostProcessor запущен" << std::endl;
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

int main()
{
    Utils postprocessor;
    postprocessor.loadConfig();

    std::string ip = postprocessor.getConfig("postprocessor.ip");
    int port = std::stoi(postprocessor.getConfig("postprocessor.port"));
    std::string output_dir = postprocessor.getConfig("postprocessor.output_dir");
    std::string proc_prefix = postprocessor.getConfig("postprocessor.processed_prefix");
    std::string bare_prefix = postprocessor.getConfig("postprocessor.bare_prefix");

    static int image_counter = 1;

    if (!postprocessor.initializeServer(ip, port))
    {
        return -1;
    }

    // Создаем экземпляр PostProcessor
    // Буфер на 90 кадров (30 кадров в каждой из 3 частей)
    // Таймаут 5 секунд
    PostProcessor videoProcessor(90, 5000);
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
                postprocessor.saveImage(proc_filename);
                std::cout << "Saved processed image: " << proc_filename << std::endl;

                // Добавляем кадр в PostProcessor
                videoProcessor.addFrame(processed_image, image_counter);

                // Подтверждаем получение первого изображения
                postprocessor.sendMessage("SEND_SECOND_IMAGE");

                // Получаем второе изображение (оригинальное)
                cv::Mat original_image = postprocessor.receiveImage();
                if (!original_image.empty())
                {
                    postprocessor.setCurrentImage(original_image);
                    std::string bare_filename = output_dir + bare_prefix + std::to_string(image_counter) + ".bmp";
                    postprocessor.saveImage(bare_filename);
                    std::cout << "Saved original image: " << bare_filename << std::endl;

                    // Подтверждаем завершение
                    postprocessor.sendMessage("DONE");

                    std::cout << "Postprocessing completed. Total: " << image_counter << std::endl;
                    image_counter++;
                }
            }
        }
    }

    videoProcessor.stop(); // Останавливаем постобработчик перед выходом

    return 0;
}