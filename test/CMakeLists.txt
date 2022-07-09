cmake_minimum_required(VERSION 3.0)

project(MYSQLTEST)

add_executable(testmysql testmysql.cpp)

target_link_libraries(testmysql mysqlclient)