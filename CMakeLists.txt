cmake_minimum_required(VERSION 3.16)
project(MyProject-modules-starter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(OpenCV REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
find_package(GTest REQUIRED) 



enable_testing()

add_subdirectory(src)
add_subdirectory(tests)


