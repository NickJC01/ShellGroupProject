cmake_minimum_required(VERSION 3.27)
project(ShellGroupProject C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(ShellGroupProject
        example/pipe_demo.c
        include/constants.h
        include/parsetools.h
        src/parsetools.c
        main.c)
