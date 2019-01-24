cmake_minimum_required(VERSION 3.11)
project(string_sort)

set(CMAKE_CXX_STANDARD 17)

include_directories(/usr/include/tbb)

add_executable(string_sort main.cpp)

target_link_libraries(string_sort PRIVATE -ltbb)