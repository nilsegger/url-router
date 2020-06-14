cmake_minimum_required(VERSION 3.16)
project(url_router)

set(CMAKE_CXX_STANDARD 17)

add_executable(url_router main.cpp path_matcher.hpp parameters.hpp)