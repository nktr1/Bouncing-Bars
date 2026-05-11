cmake_minimum_required(VERSION 3.15)
project(MiptPlayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)

add_executable(MiptPlayer main.cpp)

target_link_libraries(MiptPlayer
    fftw3
    ncurses
    "-framework CoreAudio"
    "-framework AudioToolbox"
)