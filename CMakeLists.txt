# CMakeList.txt: softnat 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
project(softnat)
include_directories(asio/include)
if(CMAKE_COMPILER_IS_GNUCXX)
    SET (CMAKE_CXX_FLAGS "-std=c++11 -pthread -g")
    add_definitions(-DASIO_STANDALONE)
else(CMAKE_COMPILER_IS_GNUCXX)
    SET (CMAKE_CXX_FLAGS "-D_WIN32_WINNT=0x0501 /EHsc")
    add_definitions(-DASIO_STANDALONE)
endif(CMAKE_COMPILER_IS_GNUCXX)

# 将源代码添加到此项目的可执行文件。
add_executable (softnat "main.cc" "softnat.cc" "softnat.h")

# TODO: 如有需要，请添加测试并安装目标。
