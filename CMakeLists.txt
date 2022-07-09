cmake_minimum_required(VERSION 3.4)

project(MY_WEBSERVER)

message(STATUS "Project:MY_WEBSERVER")

aux_source_directory(src SOURCE)

add_executable(server main.cpp ${SOURCE})

target_link_libraries(server mysqlclient pthread)