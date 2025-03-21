cmake_minimum_required(VERSION 3.15)
project(TalkoServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 ROOT_PATH 变量
set(ROOT_PATH "${CMAKE_SOURCE_DIR}")  # 则默认为当前源码目录

# 设置默认安装路径
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/output)

# 判断编译类型，并设置优化等级
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug模式：启用调试符号")
    # Debug模式下不做优化，启用调试符号
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release模式：启用优化")
    # Release模式下启用优化
    set(CMAKE_CXX_FLAGS_RELEASE "-g -O2")
else()
    message(STATUS "未知的编译类型: ${CMAKE_BUILD_TYPE}")
endif()


# 打印 ROOT_PATH 以验证
message(STATUS "项目根目录: ${ROOT_PATH}")

# 添加主项目源码
add_subdirectory(src)


# 包含头文件路径
target_include_directories(TalkoServer PRIVATE src)

# 如果启用测试，添加测试子目录
option(BUILD_TESTS "Build tests" OFF)
if (BUILD_TESTS)
    add_subdirectory(test)
endif()

# 安装规则
install(TARGETS TalkoServer DESTINATION bin)
