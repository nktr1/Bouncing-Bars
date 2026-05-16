cmake_minimum_required(VERSION 3.15)
project(MiptPlayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(MiptPlayer main.cpp)

target_link_libraries(MiptPlayer PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    fftw3                     
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework CoreFoundation"
)