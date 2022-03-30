cmake_minimum_required(VERSION 3.0)
project(remuxing)

set(CMAKE_CXX_STANDARD 17)

find_package(FFMPEG)

add_executable(remuxing remuxing.c main.cpp remuxing.h)

target_include_directories(remuxing PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(remuxing PRIVATE ${FFMPEG_LIBRARIES})