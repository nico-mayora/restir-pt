cmake_minimum_required(VERSION 3.29)
project(ReSTIR CUDA CXX C)

set(CMAKE_CUDA_STANDARD 20)
include(cmake/embed_ptx.cmake)

set(OptiX_ROOT_DIR $ENV{OptiX_INSTALL_DIR})
set(CMAKE_PREFIX_PATH $ENV{OptiX_INSTALL_DIR})

embed_ptx(
    OUTPUT_TARGET
        pathTracer_ptx
    PTX_LINK_LIBRARIES
        owl::owl
    SOURCES
        src/cuda/pathTracer.cu
)

set(sources
        src/main.cpp
        src/loader/mitsuba3.cpp
)

add_executable(pathTracer ${sources})

set(owl_dir ${PROJECT_SOURCE_DIR}/externals/owl)
add_subdirectory(${owl_dir} EXCLUDE_FROM_ALL)

target_link_libraries(pathTracer PRIVATE pathTracer_ptx owl::owl)