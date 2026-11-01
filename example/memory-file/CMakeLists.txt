cmake_minimum_required(VERSION 3.22)
project(memory-file)

set(CMAKE_CXX_STANDARD 23)

add_executable(memory-file main.cpp)

add_dependencies(memory-file fd)

target_include_directories(memory-file PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(memory-file PRIVATE fd CLI11::CLI11 fmt::fmt spdlog::spdlog )

