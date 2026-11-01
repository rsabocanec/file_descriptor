cmake_minimum_required(VERSION 3.22)
project(descriptor)

set(CMAKE_CXX_STANDARD 23)

set(SUPPORT_STACKTRACE False)

add_library(fd SHARED descriptor.cpp
        socket.cpp
        stream_socket.cpp
        tcp_socket.cpp
        bound_socket.cpp
        datagram_socket.cpp
        local_stream_socket.cpp
        udp_socket.cpp
        local_datagram_socket.cpp
        local_datagram_socket.h
        can_socket.cpp
        file.cpp
        shared_memory.cpp
        timer.cpp
        semaphores.cpp
        message_queue.cpp)

add_library(fd-static STATIC descriptor.cpp
        socket.cpp
        stream_socket.cpp
        tcp_socket.cpp
        bound_socket.cpp
        datagram_socket.cpp
        local_stream_socket.cpp
        udp_socket.cpp
        local_datagram_socket.cpp
        local_datagram_socket.h
        can_socket.cpp
        file.cpp
        shared_memory.cpp
        timer.cpp
        semaphores.cpp
        message_queue.cpp)

if(SUPPORT_STACKTRACE)
    target_compile_definitions(fd PUBLIC _STACKTRACE_SUPPORTED)
    target_link_libraries(fd INTERFACE stdc++_libbacktrace)

    target_compile_definitions(fd-static PUBLIC _STACKTRACE_SUPPORTED)
    target_link_libraries(fd-static INTERFACE stdc++_libbacktrace)
endif()

set_target_properties(fd-static PROPERTIES OUTPUT_NAME fd)

add_subdirectory(example)
