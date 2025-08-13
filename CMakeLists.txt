cmake_minimum_required(VERSION 3.16)
project(MyProject-modules-starter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(OpenCV REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
include(FetchContent)

# 下载并构建 googletest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# 避免 MSVC CRT 冲突
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

include_directories(${GTEST_INCLUDE_DIRS})

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)


