cmake_minimum_required(VERSION 3.29)
project(my_solver)

set(CMAKE_CXX_STANDARD 20)

# Aiger Library location
set(AIGER_INCLUDE_DIR "/Users/khyber/Documents/aiger-lib/aiger/include")
set(AIGER_LIB_DIR "/Users/khyber/Documents/aiger-lib/aiger/lib")

add_executable(my_solver main.cpp)

# Add Aiger headers
target_include_directories(my_solver PRIVATE ${AIGER_INCLUDE_DIR})

# Link the aiger library
target_link_libraries(my_solver PRIVATE ${AIGER_LIB_DIR}/libaiger.a)
