cmake_minimum_required(VERSION 3.15)
project(TalkoServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 ROOT_PATH 变量
set(ROOT_PATH "${CMAKE_SOURCE_DIR}")  # 则默认为当前源码目录

# 设置默认安装路径
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/output)

# 打印 ROOT_PATH 以验证
message(STATUS "项目根目录: ${ROOT_PATH}")

# 添加主项目源码
add_subdirectory(src)


# 生成可执行文件
add_executable(TalkoServer main.cpp)

# 链接 src 目录生成的库
target_link_libraries(TalkoServer PRIVATE TalkoServerLib)

# 包含头文件路径
target_include_directories(TalkoServer PRIVATE src)

# 如果启用测试，添加测试子目录
option(BUILD_TESTS "Build tests" OFF)
if (BUILD_TESTS)
    add_subdirectory(test)
endif()

# 安装规则
install(TARGETS TalkoServer DESTINATION bin)
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/include/ DESTINATION include)
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/config/ DESTINATION config)
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/database/ DESTINATION database)
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/log/ DESTINATION log)
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/service/ DESTINATION service)