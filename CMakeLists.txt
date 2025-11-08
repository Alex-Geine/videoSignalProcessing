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

    # Создаем исполняемый файл
    add_executable(${module_name}
        ${${module_name}_SOURCE_DIR}/${module_name}.h
        ${${module_name}_SOURCE_DIR}/${module_name}.cpp
    )

    # Устанавливаем выходной каталог (опционально)
    set_target_properties(${module_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Добавляем зависимости между модулями (при необходимости)
    # Например, если worker зависит от utils:
    # if(${module_name} STREQUAL "worker")
    #     target_link_libraries(${module_name} utils)
    # endif()

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
message(STATUS "Server: ${BUILD_SERVER} (${BUILD_SERVER_REAL})")
message(STATUS "Worker: ${BUILD_WORKER} (${BUILD_WORKER_REAL})")
message(STATUS "PostProcessor: ${BUILD_POSTPROCESSOR} (${BUILD_POSTPROCESSOR_REAL})")
message(STATUS "Utils: ${BUILD_UTILS} (${BUILD_UTILS_REAL})")