cmake_minimum_required(VERSION 3.16)
project(MyProject-modules-starter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


set(CMAKE_PREFIX_PATH "C:/opencv/opencv-4.5.4/x64/vc16/")
set(OpenCV_DIR "C:/opencv/opencv-4.5.4")
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories("C:/opencv/opencv-4.5.4/x64/vc16/lib")


message(STATUS "Current CMake source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

set(CMAKE_PREFIX_PATH "C:/Qt/6.7.3/msvc2019_64")  # 修改为你的 Qt 安装路径
find_package(Qt6 COMPONENTS Core Widgets Network Concurrent REQUIRED)


set(GTest_DIR "C:/GTest/lib/cmake/GTest")
set(GTEST_INCLUDE_DIR "C:/GTest/include")
set(GTEST_LIBRARY "C:/GTest/lib/gtest.lib")
set(GTEST_MAIN_LIBRARY "C:/GTest/lib/gtest_main.lib") 


find_package(GTest REQUIRED)


enable_testing()

add_subdirectory(src)
add_subdirectory(tests)


