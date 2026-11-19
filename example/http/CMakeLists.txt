cmake_minimum_required(VERSION 3.22)
project(http)

set(CMAKE_CXX_STANDARD 23)

add_library(http request.cpp)

add_dependencies(http fd)

target_include_directories(http PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(http PUBLIC fd spdlog::spdlog)
