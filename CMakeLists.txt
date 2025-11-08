cmake_minimum_required(VERSION 3.16)
project(VideoProcessingSystem VERSION 1.0.0 LANGUAGES CXX)

# Настройка версий C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции для выбора версии каждого модуля
option(BUILD_SERVER_REAL "Build server with real implementation" ON)
option(BUILD_WORKER_REAL "Build worker with real implementation" ON)
option(BUILD_POSTPROCESSOR_REAL "Build postprocessor with real implementation" ON)
option(BUILD_UTILS_REAL "Build utils with real implementation" ON)

# Опции для сборки отдельных модулей
option(BUILD_SERVER "Build server module" ON)
option(BUILD_WORKER "Build worker module" ON)
option(BUILD_POSTPROCESSOR "Build postprocessor module" ON)
option(BUILD_UTILS "Build utils module" ON)

# Автоматическая загрузка библиотек через FetchContent
include(FetchContent)

message(STATUS "Downloading and building all dependencies from source...")

# Загрузка и сборка ZeroMQ (libzmq) - используем стабильную версию
FetchContent_Declare(
    libzmq
    GIT_REPOSITORY https://github.com/zeromq/libzmq.git
    GIT_TAG v4.3.5  # Стабильная версия
)

# Настройки для libzmq для избежания проблем с компиляцией
set(BUILD_TESTS OFF CACHE BOOL "Disable libzmq tests")
set(BUILD_SHARED OFF CACHE BOOL "Build static libzmq")
set(ENABLE_DRAFTS OFF CACHE BOOL "Disable draft APIs")
set(WITH_LIBSODIUM OFF CACHE BOOL "Disable libsodium")
set(WITH_TLS OFF CACHE BOOL "Disable TLS")
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "Disable tests")

# Патчим проблемные места перед загрузкой
function(patch_zeromq_issues)
    # Эта функция будет вызвана после загрузки исходников
    # чтобы исправить проблемы с компиляцией
endfunction()

# Загружаем libzmq
FetchContent_GetProperties(libzmq)
if(NOT libzmq_POPULATED)
    FetchContent_Populate(libzmq)
    
    # Применяем патчи к исходникам ZeroMQ если нужно
    patch_zeromq_issues()
    
    # Добавляем libzmq как подпроект
    add_subdirectory(${libzmq_SOURCE_DIR} ${libzmq_BINARY_DIR})
endif()

# Загрузка и сборка cppzmq (C++ bindings)
FetchContent_Declare(
    cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    GIT_TAG v4.10.0  # Совместимая версия
)

set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "Disable cppzmq tests")

FetchContent_MakeAvailable(cppzmq)

# Загрузка и сборка yaml-cpp
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
)

# Настройки для yaml-cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Disable yaml-cpp tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Disable yaml-cpp contrib")
set(YAML_CPP_INSTALL OFF CACHE BOOL "Disable yaml-cpp installation")

FetchContent_MakeAvailable(yaml-cpp)

# Проверяем доступность библиотек
if(TARGET libzmq-static)
    set(ZMQ_LIBRARY libzmq-static)
    set(ZMQ_FOUND TRUE)
    message(STATUS "libzmq (static) successfully configured")
elseif(TARGET libzmq)
    set(ZMQ_LIBRARY libzmq)
    set(ZMQ_FOUND TRUE)
    message(STATUS "libzmq (shared) successfully configured")
elseif(TARGET zmq)
    set(ZMQ_LIBRARY zmq)
    set(ZMQ_FOUND TRUE)
    message(STATUS "libzmq (zmq) successfully configured")
else()
    # Пытаемся найти вручную
    find_library(ZMQ_LIB 
        NAMES zmq libzmq
        PATHS ${libzmq_BINARY_DIR}
    )
    if(ZMQ_LIB)
        set(ZMQ_FOUND TRUE)
        message(STATUS "Found libzmq library: ${ZMQ_LIB}")
    else()
        set(ZMQ_FOUND FALSE)
        message(FATAL_ERROR "libzmq not available")
    endif()
endif()

if(TARGET cppzmq)
    set(CPPZMQ_FOUND TRUE)
    set(CPPZMQ_INCLUDE_DIR ${cppzmq_SOURCE_DIR})
    message(STATUS "cppzmq successfully configured")
else()
    set(CPPZMQ_FOUND FALSE)
    message(FATAL_ERROR "cppzmq not available")
endif()

if(TARGET yaml-cpp)
    set(YAML_CPP_LIBRARY yaml-cpp)
    set(YAML_CPP_FOUND TRUE)
    message(STATUS "yaml-cpp successfully configured")
else()
    set(YAML_CPP_FOUND FALSE)
    message(FATAL_ERROR "yaml-cpp not available")
endif()

# Создаем интерфейсную библиотеку для ZeroMQ
add_library(zmq_interface INTERFACE)
target_link_libraries(zmq_interface INTERFACE ${ZMQ_LIBRARY} )
target_include_directories(zmq_interface INTERFACE 
    ${libzmq_SOURCE_DIR}/include
    ${CPPZMQ_INCLUDE_DIR}
)
target_compile_definitions(zmq_interface INTERFACE 
    ZMQ_FOUND
    ZMQ_STATIC  # Используем статическую линковку
)

# Функция для создания модуля
function(create_module module_name)
    # Преобразуем имя модуля в верхний регистр для опций
    string(TOUPPER ${module_name} MODULE_NAME_UPPER)
    
    # Определяем путь к исходным файлам в зависимости от выбранной версии
    if(BUILD_${MODULE_NAME_UPPER}_REAL)
        set(${module_name}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/realisation)
        message(STATUS "Building ${module_name} with REAL implementation")
    else()
        set(${module_name}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/bypass)
        message(STATUS "Building ${module_name} with BYPASS implementation")
    endif()

    # Проверяем существование директории
    if(NOT EXISTS ${${module_name}_SOURCE_DIR})
        message(FATAL_ERROR "Source directory for ${module_name} not found: ${${module_name}_SOURCE_DIR}")
    endif()

    # Проверяем наличие исходных файлов
    if(NOT EXISTS ${${module_name}_SOURCE_DIR}/${module_name}.cpp OR 
       NOT EXISTS ${${module_name}_SOURCE_DIR}/${module_name}.h)
        message(FATAL_ERROR "Source files for ${module_name} not found in ${${module_name}_SOURCE_DIR}")
    endif()

    # Создаем исполняемый файл
    add_executable(${module_name}
        ${${module_name}_SOURCE_DIR}/${module_name}.h
        ${${module_name}_SOURCE_DIR}/${module_name}.cpp
    )

    # Для модуля utils добавляем зависимости
    if(${module_name} STREQUAL "utils")
        # Добавляем yaml-cpp
        target_link_libraries(${module_name} ${YAML_CPP_LIBRARY})
        message(STATUS "Linking yaml-cpp to utils module")

        # Добавляем ZeroMQ
        target_link_libraries(${module_name} zmq_interface)
        message(STATUS "Linking ZeroMQ to utils module")

        # Добавляем определения для статической линковки
        target_compile_definitions(${module_name} PRIVATE 
            YAML_CPP_STATIC_DEFINE
            ZMQ_BUILD_DRAFT_API=0  # Отключаем draft API
            ZMQ_CUSTOM_PLATFORM_HPP
        )
        
        # Для Linux добавляем необходимые библиотеки
        if(UNIX AND NOT APPLE)
            target_link_libraries(${module_name} pthread)
        endif()
    endif()

    # Устанавливаем выходной каталог
    set_target_properties(${module_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )

    message(STATUS "Configured module: ${module_name}")
endfunction()

# Основная сборка
message(STATUS "=== Video Processing System Configuration ===")

# Собираем модули в зависимости от опций
if(BUILD_SERVER)
    create_module(server)
endif()

if(BUILD_WORKER)
    create_module(worker)
endif()

if(BUILD_POSTPROCESSOR)
    create_module(postProcessor)
endif()

if(BUILD_UTILS)
    create_module(utils)
endif()

# Вывод итоговой конфигурации
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Server: ${BUILD_SERVER} (Real: ${BUILD_SERVER_REAL})")
message(STATUS "Worker: ${BUILD_WORKER} (Real: ${BUILD_WORKER_REAL})")
message(STATUS "PostProcessor: ${BUILD_POSTPROCESSOR} (Real: ${BUILD_POSTPROCESSOR_REAL})")
message(STATUS "Utils: ${BUILD_UTILS} (Real: ${BUILD_UTILS_REAL})")
message(STATUS "YAML-cpp: ${YAML_CPP_FOUND}")
message(STATUS "ZeroMQ: ${ZMQ_FOUND} (built from source)")
message(STATUS "cppzmq: ${CPPZMQ_FOUND} (built from source)")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")