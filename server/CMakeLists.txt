cmake_minimum_required(VERSION 3.14)
project(TalkoServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 接收 ROOT_PATH 变量

set(ROOT_PATH "${CMAKE_SOURCE_DIR}")  # 则默认为当前源码目录


# 打印 ROOT_PATH 以验证
message(STATUS "项目根目录: ${ROOT_PATH}")

# 添加主项目源码
add_subdirectory(src)

# 如果启用测试，添加测试子目录
option(BUILD_TESTS "Build tests" OFF)
if (BUILD_TESTS)
    add_subdirectory(test)
endif()