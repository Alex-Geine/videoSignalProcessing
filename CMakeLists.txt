cmake_minimum_required(VERSION 3.16)
project(VideoProcessingSystem VERSION 1.0.0 LANGUAGES CXX)

# Настройка версий C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Решение проблем совместимости с зависимостями
if(CMAKE_VERSION VERSION_LESS 3.20)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

# Отключаем предупреждения о политиках для зависимостей
cmake_policy(SET CMP0048 NEW)  # Project version
cmake_policy(SET CMP0077 NEW)  # Option honor

# ============================================================================
# НАСТРОЙКА СБОРКИ И ОТЛАДКИ
# ============================================================================

# Настройка типов сборки
if(NOT CMAKE_BUILD_TYPE)
    # Можно установить "Debug" или "Realise"
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Флаги компиляции для разных типов сборки
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG -Wall -Wextra ")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG")

# Дополнительные флаги для всех сборок
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Опция для включения санитайзеров (только в Debug)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

# Санитайзеры для отладки
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -fsanitize=leak")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
    message(STATUS "Sanitizers enabled: Address, Undefined Behavior, Leak")
endif()

# ============================================================================
# ОПЦИИ ПРОЕКТА
# ============================================================================

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

message(STATUS "Video Processing System Configuration")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Sanitizers: ${ENABLE_SANITIZERS}")

# ============================================================================
# ПОИСК СИСТЕМНОГО OpenCV
# ============================================================================

message(STATUS "")
message(STATUS "================================================")
message(STATUS "Finding System OpenCV")
message(STATUS "================================================")

# Поиск OpenCV (обязательная зависимость)
find_package(OpenCV REQUIRED)

if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "
================================================================
OpenCV is REQUIRED but not found!

Please install OpenCV using your system package manager:

Ubuntu/Debian:
  sudo apt-get update && sudo apt-get install libopencv-dev

Fedora:
  sudo dnf install opencv-devel

Arch Linux:
  sudo pacman -S opencv

Or build from source with:
  https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html
================================================================
")
endif()

# ============================================================================
# ЗАГРУЗКА ОСТАЛЬНЫХ БИБЛИОТЕК
# ============================================================================

message(STATUS "")
message(STATUS "================================================")
message(STATUS "Loading other dependencies")
message(STATUS "================================================")

# Автоматическая загрузка библиотек через FetchContent
include(FetchContent)

# yaml-cpp
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Disable yaml-cpp tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Disable yaml-cpp contrib")
set(YAML_CPP_INSTALL OFF CACHE BOOL "Disable yaml-cpp installation")

FetchContent_MakeAvailable(yaml-cpp)

# ZeroMQ
FetchContent_Declare(
    libzmq
    GIT_REPOSITORY https://github.com/zeromq/libzmq.git
    GIT_TAG v4.3.5
)

# Устанавливаем опции до вызова MakeAvailable
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ENABLE_DRAFTS OFF CACHE BOOL "" FORCE)
set(WITH_LIBSODIUM OFF CACHE BOOL "" FORCE)
set(WITH_TLS OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(libzmq)

# cppzmq
FetchContent_Declare(
    cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    GIT_TAG v4.10.0
)

set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "Disable cppzmq tests")
FetchContent_MakeAvailable(cppzmq)

# ============================================================================
# ПРОВЕРКА БИБЛИОТЕК
# ============================================================================

message(STATUS "Checking available targets...")

# ZeroMQ - проверяем все возможные имена целей
set(ZMQ_FOUND FALSE)
if(TARGET libzmq)
    set(ZMQ_LIBRARY libzmq)
    set(ZMQ_FOUND TRUE)
    message(STATUS "Found ZeroMQ target: libzmq")
elseif(TARGET libzmq-static)
    set(ZMQ_LIBRARY libzmq-static)
    set(ZMQ_FOUND TRUE)
    message(STATUS "Found ZeroMQ target: libzmq-static")
elseif(TARGET zmq)
    set(ZMQ_LIBRARY zmq)
    set(ZMQ_FOUND TRUE)
    message(STATUS "Found ZeroMQ target: zmq")
else()
    # Выводим все доступные цели для отладки
    get_property(targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    message(STATUS "All available targets: ${targets}")
    message(FATAL_ERROR "libzmq not available as any known target")
endif()

# cppzmq
if(TARGET cppzmq)
    set(CPPZMQ_FOUND TRUE)
    set(CPPZMQ_INCLUDE_DIR ${cppzmq_SOURCE_DIR})
    message(STATUS "Found cppzmq target: cppzmq")
else()
    message(FATAL_ERROR "cppzmq not available")
endif()

# yaml-cpp
if(TARGET yaml-cpp)
    set(YAML_CPP_LIBRARY yaml-cpp)
    set(YAML_CPP_FOUND TRUE)
    message(STATUS "Found yaml-cpp target: yaml-cpp")
else()
    message(FATAL_ERROR "yaml-cpp not available")
endif()

# ============================================================================
# ИНТЕРФЕЙСНЫЕ БИБЛИОТЕКИ
# ============================================================================

# ZeroMQ интерфейс
add_library(zmq_interface INTERFACE)
target_link_libraries(zmq_interface INTERFACE ${ZMQ_LIBRARY})
target_include_directories(zmq_interface INTERFACE 
    ${libzmq_SOURCE_DIR}/include
    ${CPPZMQ_INCLUDE_DIR}
)

# Добавляем флаги отладки для ZeroMQ
target_compile_definitions(zmq_interface INTERFACE 
    $<$<CONFIG:Debug>:ZMQ_BUILD_DRAFT_API=0>
)

# OpenCV интерфейс
add_library(opencv_interface INTERFACE)
target_link_libraries(opencv_interface INTERFACE ${OpenCV_LIBS})
target_include_directories(opencv_interface INTERFACE ${OpenCV_INCLUDE_DIRS})

# ============================================================================
# ФУНКЦИЯ ДЛЯ СОЗДАНИЯ МОДУЛЕЙ
# ============================================================================

function(create_module module_name)
    string(TOUPPER ${module_name} MODULE_NAME_UPPER)
    
    if(BUILD_${MODULE_NAME_UPPER}_REAL)
        set(${module_name}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/realisation)
        message(STATUS "Building ${module_name} with REAL implementation")
    else()
        set(${module_name}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/bypass)
        message(STATUS "Building ${module_name} with BYPASS implementation")
    endif()

    if(NOT EXISTS ${${module_name}_SOURCE_DIR})
        message(FATAL_ERROR "Source directory for ${module_name} not found: ${${module_name}_SOURCE_DIR}")
    endif()

    # Для utils создаем библиотеку, для других модулей - исполняемые файлы
    if(${module_name} STREQUAL "utils")
        add_library(${module_name}
            ${${module_name}_SOURCE_DIR}/${module_name}.h
            ${${module_name}_SOURCE_DIR}/${module_name}.cpp
        )
        
    else()
        add_executable(${module_name}
            ${${module_name}_SOURCE_DIR}/${module_name}.h
            ${${module_name}_SOURCE_DIR}/${module_name}.cpp
        )
        
        set_target_properties(${module_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endif()

    # ОБЩИЕ НАСТРОКИ ДЛЯ ВСЕХ МОДУЛЕЙ
    if(${module_name} STREQUAL "utils")
        # Зависимости для utils
        target_link_libraries(${module_name}
            ${YAML_CPP_LIBRARY}
            zmq_interface
            opencv_interface
        )
        
        # Добавляем include директории для utils
        target_include_directories(${module_name} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/realisation
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/bypass
        )
        
        # Системные библиотеки
        if(UNIX AND NOT APPLE)
            target_link_libraries(${module_name} 
                pthread 
                dl 
                m
                stdc++fs
            )
        endif()
        
        message(STATUS "Linked to utils: yaml-cpp, ZeroMQ, OpenCV")
        
    else()
        # ДЛЯ СЕРВЕРА, WORKER И POSTPROCESSOR - линкуемся с utils
        
        # Определяем откуда брать utils.h в зависимости от версии utils
        if(BUILD_UTILS_REAL)
            set(UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utils/realisation)
        else()
            set(UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utils/bypass)
        endif()
        
        target_link_libraries(${module_name} utils)
        
        # Добавляем include директории
        target_include_directories(${module_name} PRIVATE 
            ${UTILS_INCLUDE_DIR}
            ${OpenCV_INCLUDE_DIRS}
            ${libzmq_SOURCE_DIR}/include
            ${CPPZMQ_INCLUDE_DIR}
        )
        
        # Системные библиотеки
        if(UNIX AND NOT APPLE)
            target_link_libraries(${module_name} 
                pthread 
                dl 
                m
                stdc++fs
            )
        endif()
        
        message(STATUS "Linked ${module_name} to utils library (include: ${UTILS_INCLUDE_DIR})")
    endif()

    # ДОБАВЛЯЕМ ФЛАГИ ОТЛАДКИ ДЛЯ ВСЕХ ЦЕЛЕЙ
    target_compile_definitions(${module_name} PRIVATE 
        $<$<CONFIG:Debug>:DEBUG=1>
    )
    
    # Включаем отладочную информацию для всех целей
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${module_name} PRIVATE -g -O0)
    endif()
    
    message(STATUS "Configured module: ${module_name}")
endfunction()

# ============================================================================
# ОСНОВНАЯ СБОРКА
# ============================================================================

message(STATUS "")
message(STATUS "================================================")
message(STATUS "Building Application Modules")
message(STATUS "================================================")

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

# ============================================================================
# ФИНАЛЬНЫЙ ОТЧЕТ
# ============================================================================

message(STATUS "")
message(STATUS "================================================")
message(STATUS "Build Configuration Summary")
message(STATUS "================================================")
message(STATUS "Server: ${BUILD_SERVER} (Real: ${BUILD_SERVER_REAL})")
message(STATUS "Worker: ${BUILD_WORKER} (Real: ${BUILD_WORKER_REAL})")
message(STATUS "PostProcessor: ${BUILD_POSTPROCESSOR} (Real: ${BUILD_POSTPROCESSOR_REAL})")
message(STATUS "Utils: ${BUILD_UTILS} (Real: ${BUILD_UTILS_REAL})")
message(STATUS "YAML-cpp: ${YAML_CPP_FOUND}")
message(STATUS "ZeroMQ: ${ZMQ_FOUND} (${ZMQ_LIBRARY})")
message(STATUS "cppzmq: ${CPPZMQ_FOUND}")
message(STATUS "OpenCV: ${OpenCV_VERSION} (REQUIRED)")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Sanitizers: ${ENABLE_SANITIZERS}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "================================================")