cmake_minimum_required(VERSION 3.18)

project(Facial-Attendence-Lambda)

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDebug)

set(UTILS_LIB_IN ${CMAKE_CURRENT_SOURCE_DIR}/../utils/lib)
cmake_path(NORMAL_PATH UTILS_LIB_IN OUTPUT_VARIABLE UTILS_LIB)
add_subdirectory(../utils ${UTILS_LIB})

set(SRCS
    src/main.cpp
)

add_executable(falambda ${SRCS})
target_link_libraries( falambda utils)
target_compile_features( falambda PRIVATE cxx_std_17)
target_include_directories(falambda PUBLIC "include/")