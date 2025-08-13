cmake_minimum_required(VERSION 3.16)
project(MyProject-modules-starter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(OpenCV REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
find_package(GTest REQUIRED) 

if (NOT GTEST_INCLUDE_DIRS)
    set(GTEST_INCLUDE_DIRS "$ENV{GTEST_INCLUDE_DIR}")
endif()

if (NOT GTEST_LIBRARIES)
    set(GTEST_LIBRARIES "$ENV{GTEST_LIBRARY}" "$ENV{GTEST_MAIN_LIBRARY}")
endif()

include_directories(${GTEST_INCLUDE_DIRS})

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)


