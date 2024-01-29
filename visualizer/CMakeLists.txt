cmake_minimum_required(VERSION 3.18)

project(Facial-Attendence-Visulizer)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(UTILS_LIB_IN ${CMAKE_CURRENT_SOURCE_DIR}/../utils/lib)
cmake_path(NORMAL_PATH UTILS_LIB_IN OUTPUT_VARIABLE UTILS_LIB)
add_subdirectory(../utils ${UTILS_LIB})

set(SRCS
    src/VisulizerApp.cpp
)

set(CINDER_DIR "C:/tools/Cinder")
include_directories(${CINDER_DIR}/include)
link_directories(${CINDER_DIR}/lib/msw/x64)
link_directories(${CINDER_DIR}/lib/msw/x64/Debug/v143)

add_executable(favis ${SRCS})
target_link_libraries( favis utils cinder)
target_compile_features(favis PRIVATE cxx_std_17)
target_include_directories(favis PUBLIC "include/" "../utils/include/")