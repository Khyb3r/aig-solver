cmake_minimum_required(VERSION 3.29)
project(my_solver)

set(CMAKE_CXX_STANDARD 20)

add_executable(my_solver main.cpp
        aig.h
        Arena.h
        Solver/Solver.cpp
        Solver/Solver.h
        aiger.c
)

target_include_directories(my_solver PRIVATE ${CMAKE_SOURCE_DIR})