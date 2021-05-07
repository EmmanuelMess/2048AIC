cmake_minimum_required(VERSION 3.19)
project(AI2048 C)

set(CMAKE_C_STANDARD 11)

add_executable(AI2048 main.c board.c board.h pcg_basic.c pcg_basic.h)


#find_package(OpenMP)
#if (OPENMP_FOUND)
#  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
#endif()

target_link_libraries(AI2048 m)