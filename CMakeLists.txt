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

# Автоматическая загрузка yaml-cpp через FetchContent
include(FetchContent)

message(STATUS "Downloading and building yaml-cpp from source...")

# Используем правильные теги yaml-cpp
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0  # Правильный формат тега
)

# Альтернативные варианты тегов:
# GIT_TAG 0.7.0-release
# GIT_TAG 0.6.3
# GIT_TAG master  # последняя версия из master ветки

# Настройки для yaml-cpp перед загрузкой
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Disable yaml-cpp tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Disable yaml-cpp contrib")
set(YAML_CPP_INSTALL OFF CACHE BOOL "Disable yaml-cpp installation")

# Загружаем и делаем доступным yaml-cpp
FetchContent_MakeAvailable(yaml-cpp)

# Проверяем, что yaml-cpp доступен
if(TARGET yaml-cpp)
    set(YAML_CPP_LIBRARY yaml-cpp)
    set(YAML_CPP_FOUND TRUE)
    message(STATUS "yaml-cpp successfully configured: ${YAML_CPP_LIBRARY}")
else()
    # Альтернативный вариант: попробовать найти установленную версию
    find_package(yaml-cpp QUIET)
    if(yaml-cpp_FOUND)
        set(YAML_CPP_LIBRARY yaml-cpp)
        set(YAML_CPP_FOUND TRUE)
        message(STATUS "Using system yaml-cpp library")
    else()
        message(WARNING "yaml-cpp not available, utils module will be built without it")
        set(YAML_CPP_FOUND FALSE)
    endif()
endif()

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

    # Для модуля utils добавляем yaml-cpp зависимости
    if(${module_name} STREQUAL "utils" AND YAML_CPP_FOUND)
        if(TARGET ${YAML_CPP_LIBRARY})
            target_link_libraries(${module_name} ${YAML_CPP_LIBRARY})
            message(STATUS "Linking yaml-cpp to utils module")
        else()
            target_link_libraries(${module_name} ${YAML_CPP_LIBRARY})
            message(STATUS "Linking system yaml-cpp to utils module")
        endif()
    elseif(${module_name} STREQUAL "utils")
        message(WARNING "Building utils module without yaml-cpp support")
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
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")