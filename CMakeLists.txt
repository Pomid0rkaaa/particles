cmake_minimum_required(VERSION 3.16)
project(particles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(particles src/main.cpp)

target_link_libraries(particles raylib)
