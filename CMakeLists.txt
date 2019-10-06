cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(fourth-paragraph VERSION 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

include(thirdparty/thirdparty.cmake)

add_subdirectory(src)

