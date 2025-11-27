# Система Распределенной Обработки Видео Сигнала (РОВс)

Данный документ описывает работу с репозиторием videoSignalProcessing.

Данный репозиторий хранит исходный код для четвертого задания с Минеевым.

## Правила работы с репозиторием:
- Работа с репозиторием происходит в собственной ветке, унаследованной от main

- Ветка main содержит исходный код с заглушками, которые необходимы для проверки и демонстрации
работы всего проекта в целом. Поэтому, изменения в эту ветку вносить нельзя.


## Настройка окружения:

### 1) Установка WSL
- В Microsoft Store пишем "wsl" и находим Ubuntu 24.04.1 LTS. Устанавливаем.
- Запускаем установленное приложение.
- Ждем, пока установится.
- Вводим login и пароль для входа в систему
- Если все прошло успешно, в терминале будет написано:

```
Installation successful!
```
- В противном случае, можно попробовать обратится ко мне (там бывают приколы с установкой wsl)

### 2) Установка и настройка Visual Studio Code
- Устанавливаем Visual Studio Code
- Открываем приложение и устанавливаем необходимые дополнения к нему. Делается это через
вкладку Extensions (Ctrl + Shift + X). 
Список дополнений:
    1. WSL - для работы с wsl
    2. C/C++
    3. C/C++ Extension Pack
    4. CMake Tools
- Далее необходимо подключиться к wsl. Жмем "Ctrl + Shift + P", открывается меню сверху.
В нем пишем: 
```
WSL: Connect to WSL using Distro...
```
Выбираем Ubuntu и подключаемся.

- Во вкладке Extensions снова загружаем нужные дополнения (они уникальные для каждого WSL).

**NOTE:**  Терминал открывается на "Ctrl + ~", между папками можно переключаться через "Ctrl + K + O".

3) Настройка самой Ubuntu.
Для дальнейшей работы нам необходимо следующее:
* Компилятор gcc/g++
* Система для автоматизации сборки CMAKE
* pkg-config для сборки zmq
* OpenCV - для обработки изображений
* gdb - для отладки  

Обновим систему:

```
sudo apt update
sudo apt upgrade
```

Установим gcc, g++
```
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install gcc-13 g++-13
gcc-13 --version # Проверка установки
```

Установим cmake:
```
sudo apt  install cmake
```

Установим pkg-config:
```
sudo apt-get install pkg-config
```

Установим OpenCV:
```
# Минимальные зависимости для OpenCV
sudo apt-get install -y build-essential cmake git pkg-config
sudo apt-get install -y libjpeg-dev libtiff5-dev libpng-dev
sudo apt-get install -y libv4l-dev v4l-utils
sudo apt-get install -y libgtk-3-dev
sudo apt-get install -y libbsd-dev

# Install OpenCV
sudo apt install libopencv-dev
```

Установим gdb:
```
sudo apt-get install gdb
```

### 4) Клонирование репозитория и создание своей ветки:
```
git clone https://github.com/Alex-Geine/videoSignalProcessing.git
cd videoSignalProcessing/
git checkout -b name_of_new_branch
```

### 5) Сборка проекта

Cтруткура проекта:
```
videoSignalProcessing/
├── CMakeLists.txt           - инструкции для сборки
├── config.yaml              - конфигурация модулей
├── server/                  - код для Сервера
│   ├── realization/         - Сюда нужно вставить реализацию
│   │   ├── server.h
│   │   └── server.cpp
│   └── bypass/              - Здесь хранится заглушка
│       ├── server.h
│       └── server.cpp
├── worker/                  - код для Обработчика
│   ├── realization/         - Сюда нужно вставить реализацию
│   │   ├── worker.h
│   │   └── worker.cpp
│   └── bypass/              - Здесь хранится заглушка
│       ├── worker.h
│       └── worker.cpp
├── postProcessor/          - код для Пост-обработчика
│   ├── realization/        - Сюда нужно вставить реализацию
│   │   ├── postProcessor.h
│   │   └── postProcessor.cpp
│   └── bypass/             - Здесь хранится заглушка
│       ├── postProcessor.h
│       └── postProcessor.cpp
├── utils/                  - код для вспомогательных инструментов
│   ├── realization/        - Сюда нужно вставить реализацию
│   │   ├── utils.h
│   │   └── utils.cpp
│   └── bypass/             - Здесь хранится заглушка
│       ├── utils.h
│       └── utils.cpp
├── pic/                    - Здесь хранятся фото

```

При сборке проекта предусмотрены следующие возможности:

* Можно выборочно собирать модули
* Можно для каждого модуля задавать его тип: реализация/заглушка

Для этого в CmakeLists.txt есть специальные флаги:

```
# Опции для выбора версии каждого модуля
option(BUILD_SERVER_REAL "Build server with real implementation" OFF)
option(BUILD_WORKER_REAL "Build worker with real implementation" OFF)
option(BUILD_POSTPROCESSOR_REAL "Build postprocessor with real implementation" OFF)
option(BUILD_UTILS_REAL "Build utils with real implementation" OFF)

# Опции для сборки отдельных модулей
option(BUILD_SERVER "Build server module" ON)
option(BUILD_WORKER "Build worker module" ON)
option(BUILD_POSTPROCESSOR "Build postprocessor module" ON)
option(BUILD_UTILS "Build utils module" ON)

```

Также есть флаг для сборки в Release/Debug
```
# Настройка типов сборки
if(NOT CMAKE_BUILD_TYPE)
    # Можно установить "Debug" или "Release"
    set(CMAKE_BUILD_TYPE Debug)
endif()
```

Поправив при надобности CMakeLists.txt можно приступить к сборке:
* Найти доступные компиляторы: Ctrl + Shift + P -> CMake: Scan for kits
* Установить необходимый компилятор: Ctrl + Shift + P -> CMake: Select a Kit
* Сконфигурировать проект: Ctrl + Shift + P -> CMake: Configure
* Собрать проект: Ctrl + Shift + P -> CMake: Build

> [!CAUTION]
> Если встречается ошибка вида:
```
CMake Error at build/_deps/yaml-cpp-src/CMakeLists.txt:2 (cmake_minimum_required):
  Compatibility with CMake < 3.5 has been removed from CMake.
```
то собрать проект через `./unsafe_build.sh`.

Исходники будут находится в папке $build/bin/$.

Запустить приложение можно с помощью скрипта $run.sh$:

```
sh run.sh name_of_module

# Например
sh run.sh server
```

Или просто запустив исходник:
```
./build/bin/server
```


### 6) Отладка
* Через значок с жучком на интерфейсе VSCode (или Ctrl + Shift + D)
* ИЛИ Ctrl + Shift + P -> Cmake: Debug -> Выбираем нужный модуль