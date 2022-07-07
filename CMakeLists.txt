cmake_minimum_required(VERSION 3.0)

project(MY_WEBSERVER VERSION 1.0)

message(STATUS "Project:MY_WEBSERVER")

include_directories(include)

aux_source_directory(src SOURCE)

add_executable(server main.cpp ${SOURCE})

target_link_libraries(server pthread)