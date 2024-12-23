cmake_minimum_required(VERSION 3.29)
project(ReSTIR CUDA CXX C)

set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
include(cmake/embed_ptx.cmake)

set(glfw_dir ${PROJECT_SOURCE_DIR}/externals/glfw)
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
        src/pt-math.cpp
        src/viewer.cpp
)

add_executable(pathTracer ${sources})

include_directories(
        ${glfw_dir}/include
        ${owl_dir}/samples/common # For OWLViewer
)
add_subdirectory(${glfw_dir})

set(owl_dir ${PROJECT_SOURCE_DIR}/externals/owl)
add_subdirectory(${owl_dir} EXCLUDE_FROM_ALL)

set(tinyxml2_dir ${PROJECT_SOURCE_DIR}/externals/tinyxml2)
add_subdirectory(${tinyxml2_dir} EXCLUDE_FROM_ALL)

target_link_libraries(pathTracer PRIVATE pathTracer_ptx owl::owl owl_viewer tinyxml2::tinyxml2)