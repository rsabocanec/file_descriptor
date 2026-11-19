cmake_minimum_required(VERSION 3.22)
project(http-server)

set(CMAKE_CXX_STANDARD 23)

add_executable(http-server main.cpp)

add_dependencies(http-server http fd)

target_include_directories(http-server PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(http-server PRIVATE http fd ssl crypto CLI11::CLI11 spdlog::spdlog fmt::fmt)
