cmake_minimum_required(VERSION 3.18)

project(Facial-Attendence-Utils)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(SRCS
    src/DBConnection.cpp
    src/Entity.cpp
    src/Map.cpp
)

find_package(Boost REQUIRED )
find_package(OpenSSL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

set(CINDER_DIR "C:/tools/Cinder")

set(UTIL_INCLUDES 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${Boost_INCLUDE_DIR}
    ${CINDER_DIR}/include
)

set(UTIL_LINK_DIRS 
    ${CINDER_DIR}/lib/msw/x64
    ${CINDER_DIR}/lib/msw/x64/Debug/v143
)

add_library(utils ${SRCS})
target_link_directories(utils PUBLIC ${UTIL_LINK_DIRS})
target_link_libraries( utils PUBLIC OpenSSL::SSL fmt::fmt cinder Eigen3::Eigen)
target_compile_features( utils PUBLIC cxx_std_17)
target_include_directories(utils PUBLIC ${UTIL_INCLUDES})
target_compile_definitions(utils PUBLIC -D_WIN32_WINNT=0x0601)
