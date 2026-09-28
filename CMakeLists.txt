cmake_minimum_required(VERSION 3.30)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
project(PathTracer)

# glad
add_library(glad STATIC ${CMAKE_SOURCE_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/include)

# imgui
file(GLOB_RECURSE IMGUI_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/include/imgui/*.cpp)
list(FILTER IMGUI_SOURCES EXCLUDE REGEX ".*freetype.*")
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/include/imgui)
target_link_libraries(imgui PUBLIC glfw)

function(set_common_target_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /EHsc /openmp)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -fopenmp)
    endif()
    target_compile_features(${target} PRIVATE cxx_std_17)
endfunction()

# Get libraries directly from their repo
include(FetchContent)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(nfd GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git GIT_TAG v1.3.0)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.5.1)
FetchContent_MakeAvailable(nfd glfw)

find_package(CUDAToolkit REQUIRED)
find_package(OpenMP REQUIRED)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

if (NOT ENABLE_TENSORRT)
    list(FILTER CORE_SOURCES EXCLUDE REGEX "/src/cuda_cpp/")
endif()

add_library(pathtrace_core STATIC ${CORE_SOURCES})
set_common_target_options(pathtrace_core)
target_include_directories(pathtrace_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(pathtrace_core PUBLIC glad imgui glfw nfd CUDA::cudart OpenMP::OpenMP_CXX)

option(ENABLE_TENSORRT "Enable TensorRT-based features (NVIDIA only)" OFF)

if(ENABLE_TENSORRT)
    find_path(TENSORRT_INCLUDE_DIR NvInfer.h
        HINTS ${TensorRT_DIR}/include /usr/include/x86_64-linux-gnu)
    find_library(TENSORRT_LIBRARY nvinfer
        HINTS ${TensorRT_DIR}/lib /usr/lib/x86_64-linux-gnu)

    if(NOT TENSORRT_INCLUDE_DIR OR NOT TENSORRT_LIBRARY)
        message(FATAL_ERROR "TensorRT not found. Pass -DTensorRT_DIR=/path/to/TensorRT")
    endif()

    add_library(TensorRT::nvinfer UNKNOWN IMPORTED)
    set_target_properties(TensorRT::nvinfer PROPERTIES IMPORTED_LOCATION "${TENSORRT_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${TENSORRT_INCLUDE_DIR}")

    target_compile_definitions(pathtrace_core PUBLIC ENABLE_TENSORRT)
endif()

add_executable(myprogram src/main.cpp)
set_common_target_options(myprogram)
target_link_libraries(myprogram PRIVATE pathtrace_core)