cmake_minimum_required(VERSION 3.22)
project(https-server)

set(CMAKE_CXX_STANDARD 23)

add_executable(https-server main.cpp)

add_dependencies(https-server http fd)

target_include_directories(https-server PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(https-server PRIVATE http fd ssl crypto CLI11::CLI11 spdlog::spdlog fmt::fmt)
