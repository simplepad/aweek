cmake_minimum_required(VERSION 3.22)
project(aweek_c C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_COMPILER gcc)

set(CMAKE_C_FLAGS "-Wall -g")

add_executable(aweek_c src/main.c src/anime_functions.c include/anime_functions.h)

find_package(json-c CONFIG)
target_link_libraries(aweek_c PRIVATE json-c::json-c)
