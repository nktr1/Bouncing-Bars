cmake_minimum_required(VERSION 3.15)
project(MiptPlayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Добавляем пути Homebrew для поиска заголовочных файлов и библиотек
# Это критично для Apple Silicon (M1/M2/M3)
include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)

# 2. Ищем SFML 3
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(MiptPlayer main.cpp)

# 3. Линковка
target_link_libraries(MiptPlayer PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    fftw3                     # Теперь CMake найдет его в /opt/homebrew/lib
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework CoreFoundation"
)