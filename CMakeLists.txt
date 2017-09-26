# minimal required cmake version
cmake_minimum_required(VERSION 3.7)

# you can set your own project name
project(vas17)

# this should not be changed
set(CMAKE_C_STANDARD 99)

# add your source files
set(SOURCE_FILES
        src/dist.c
        src/dist.h
        src/enc.c
        src/enc.h
        src/led.c
        src/led.h
        src/main.c
        src/mot.c
        src/mot.h
        src/pir.c
        src/pir.h
        src/pulse_out.c
        src/pulse_out.h)

# create a fake library target
add_library(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

# include naos include paths
add_subdirectory(naos)
