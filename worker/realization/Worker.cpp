#include "scanner_darkly_effect.hpp"
#include <iostream>
#include <string>

int main() {
    try {
        // Путь к изображению
        std::string image_path = "C:/111/123.jpg";

        // Загрузка изображения
        cv::Mat input_image = cv::imread(image_path);

        if (input_image.empty())
        {
            std::cerr << "Не удалось загрузить изображение по пути: " << image_path << std::endl;
            return -1;
        }

        std::cout << "Изображение загружено успешно." << std::endl;
        std::cout << "Размер изображения: " << input_image.cols << "x" << input_image.rows << std::endl;

        // Создание экземпляра эффекта
        ScannerDarklyEffect effect;

        // Настройка эффекта Scanner Darkly
        effect.setCannyThresholds(60, 160);        // Пороги для детектора границ Кэнни
        effect.setGaussianKernelSize(3);           // Размер ядра размытия Гаусса
        effect.setDilationKernelSize(0);           // Размер ядра дилатации (0 = нет дилатации)
        effect.setColorQuantizationLevels(8);      // Уровни квантования цвета
        effect.setBlackContours(true);             // Использовать черные контуры

        // Применение эффекта
        std::cout << "Применение эффекта Scanner Darkly..." << std::endl;
        cv::Mat result = effect.applyEffect(input_image);

        // Сохранение результата
        std::string output_path = "C:/111/processed_image.jpg";
        bool saved = cv::imwrite(output_path, result);

        if (saved) 
        {
            std::cout << "Результат сохранен в: " << output_path << std::endl;
        }
        else
        {
            std::cerr << "Не удалось сохранить результат!" << std::endl;
        }
                     
        std::cout << "Нажмите любую клавишу для выхода..." << std::endl;
        cv::waitKey(0);

        cv::destroyAllWindows();

    }
    catch (const std::exception& e) 
    {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}