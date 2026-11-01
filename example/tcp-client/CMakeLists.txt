cmake_minimum_required(VERSION 3.22)
project(tcp-client)

set(CMAKE_CXX_STANDARD 23)

add_executable(tcp-client main.cpp)

add_dependencies(tcp-client fd)

target_include_directories(tcp-client PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(tcp-client PRIVATE fd CLI11::CLI11 spdlog::spdlog fmt::fmt)
