cmake_minimum_required(VERSION 3.10.0)
project(generator VERSION 0.1.0 LANGUAGES C CXX)

SET(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_executable(${PROJECT_NAME} main.cpp)

add_subdirectory(tests)
