cmake_minimum_required(VERSION 3.18)
project(Semestralka C)

set(CMAKE_C_STANDARD 99)

add_executable(Semestralka-Server Server/main.c Server/users.c Server/users.h colors.h maxLimits.h)
target_link_libraries(Semestralka-Server pthread)

add_executable(Semestralka-Client Client/main.c Client/operations.c Client/operations.h maxLimits.h)
target_link_libraries(Semestralka-Client pthread)