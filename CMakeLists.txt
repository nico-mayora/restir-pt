cmake_minimum_required(VERSION 3.29)
project(ReSTIR CUDA)

set(CMAKE_CUDA_STANDARD 20)

add_executable(ReSTIR main.cu)

set_target_properties(ReSTIR PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)
