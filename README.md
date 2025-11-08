Данный документ описывает работу с репозиторием videoSignalProcessing.

Данный репозиторий хранит исходный код для четвертого задания с Минеевым.

Правила работы с репозиторием:
- Работа с репозиторием происходит в собственной ветке, унаследованной от main

- Ветка main содержит исходный код с заглушками, которые необходимы для проверки и демонстрации
работы всего проекта в целом. Поэтому, изменения в эту ветку вность нельзя.


Настройка окружения:

1) Установка WSL
- В Microsoft Store пишем "wsl" и находим Ubuntu 24.04.1 LTS. Устанавливаем.
- Запускаем установленное приложение.
- Ждем, пока установится.
- Вводим login и пароль для входа в систему
- Если все прошло успешно, в терминале будет написано:

```
Installation successful!
```
- В противном случае, можно попробовать обратится ко мне (там бывают приколы с установкой wsl)

2) Установка и настройка Visual Studio Code
- Устанавливаем Visual Studio Code
- Открываем приложение и устанавливаем необходимые дополнения к нему. Делается это через
вкладку Extensions. 
Список дополнений:
    1. WSL - для работы с wsl
    2. C/C++
    3. C/C++ Extension Pack
    4. CMake Tools
- Далее необходимо подключиться к wsl. Жмем "Cntr + Shift + P", открывается меню сверху.
В нем пишем: 
```
WSL: Connect to WSL using Distro...
```
Выбираем Ubuntu и подключаемся.

- Во вкладке Extensions снова загружаем нужные дополнения (они уникальные для каждого WSL).

NOTE: Терминал открывается на "Cntrl + ~", между папками можно переключаться через "Cntrl + K + O".

3) Настройка самой Ubuntu.
Для дальнейшей работы нам необходимо следующее:
* Компилятор gcc/g++
* Система для автоматизации сборки CMAKE     

```
sudo apt update
sudo apt upgrade
```

Установка gcc, g++
```
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install gcc-13 g++-13
gcc-13 --version # Проверка установки
```

Установка cmake:
```
sudo apt  install cmake
```

4) Клонирование репозитория и создание своей ветки:
```
git clone https://github.com/Alex-Geine/videoSignalProcessing.git
cd videoSignalProcessing/
git checkout -b name_of_new_branch
```

5) Сборка проекта


струткура проекта:
```
videoSygnalProcessing/
├── CMakeLists.txt
├── server/
│   ├── realisation/
│   │   ├── server.h
│   │   └── server.cpp
│   └── bypass/
│       ├── server.h
│       └── server.cpp
├── worker/
│   ├── realisation/
│   │   ├── worker.h
│   │   └── worker.cpp
│   └── bypass/
│       ├── worker.h
│       └── worker.cpp
├── postProcessor/
│   ├── realisation/
│   │   ├── postProcessor.h
│   │   └── postProcessor.cpp
│   └── bypass/
│       ├── postProcessor.h
│       └── postProcessor.cpp
└── utils/
    ├── realisation/
    │   ├── utils.h
    │   └── utils.cpp
    └── bypass/
        ├── utils.h
        └── utils.cpp
```