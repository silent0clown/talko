cmake_minimum_required(VERSION 3.15)
project(TalkoServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 ROOT_PATH 变量
set(ROOT_PATH "${CMAKE_SOURCE_DIR}")  # 则默认为当前源码目录
message(STATUS "项目根目录: ${ROOT_PATH}")

# 设置编译操作系统(SYSTEM)和芯片架构信息(Architecture)
# 获取操作系统
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(SYSTEM "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(SYSTEM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(SYSTEM "macOS")
else()
    set(SYSTEM "unknown")
endif()

# 获取 CPU 架构
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(ARCHITECTURE "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686|i386")
    set(ARCHITECTURE "x86")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
    set(ARCHITECTURE "ARMv7")
else()
    set(ARCHITECTURE "unknown")
endif()
# 输出编译环境信息
message(STATUS "Building for ${SYSTEM} on ${ARCHITECTURE}")

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


# 添三方库头文件路径
set(THIRD_PARTY_PATH ${ROOT_PATH}/third_party/${SYSTEM}/${ARCHITECTURE})

include_directories(${THIRD_PARTY_PATH}/openssl/include)
include_directories(${THIRD_PARTY_PATH}/mysqlconn/include)
include_directories(${THIRD_PARTY_PATH}/libuv/include)
include_directories(${THIRD_PARTY_PATH}/uwebsockets/include)
include_directories(${THIRD_PARTY_PATH}/uwebsockets/uSockets/include)
include_directories(${THIRD_PARTY_PATH}/yaml-cpp/include)

if (0)
# 添加库
add_library(crypto STATIC IMPORTED)
add_library(ssl STATIC IMPORTED)
add_library(mysqlcppconn8 STATIC IMPORTED)
add_library(libuv STATIC IMPORTED)


# 添加库路径
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${THIRD_PARTY_PATH}/openssl/lib/libcrypto.a)
set_target_properties(ssl PROPERTIES IMPORTED_LOCATION ${THIRD_PARTY_PATH}/openssl/lib/libssl.a)
set_target_properties(mysqlcppconn8 PROPERTIES IMPORTED_LOCATION ${THIRD_PARTY_PATH}/mysqlconn/lib/libmysqlcppconn8.a)
endif()


add_library(crypto SHARED IMPORTED)
add_library(ssl SHARED IMPORTED)
add_library(mysqlcppconn8 SHARED IMPORTED)
add_library(libuv SHARED IMPORTED)
add_library(usocket STATIC IMPORTED)
add_library(yaml-cpp STATIC IMPORTED)


# 添加库路径
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${THIRD_PARTY_PATH}/openssl/lib/libcrypto.so)
set_target_properties(ssl PROPERTIES IMPORTED_LOCATION ${THIRD_PARTY_PATH}/openssl/lib/libssl.so)
set_target_properties(mysqlcppconn8 PROPERTIES IMPORTED_LOCATION ${THIRD_PARTY_PATH}/mysqlconn/lib/libmysqlcppconn8.so.2.8.0.33)
set_target_properties(usocket PROPERTIES IMPORTED_LOCATION {THIRD_PARTY_PATH}/uwebsockets/uSockets/libuSockets.a)
set_target_properties(yaml-cpp PROPERTIES IMPORTED_LOCATION {THIRD_PARTY_PATH}/yaml-cpp/lib/libyaml-cpp.a)
# 添加主项目子文件夹
add_subdirectory(src)


# 包含头文件路径
target_include_directories(whisp_server_proc PRIVATE src)

# 如果启用测试，添加测试子目录
option(BUILD_TESTS "Build tests" OFF)
if (BUILD_TESTS)
    add_subdirectory(test)
endif()

# 安装规则
install(TARGETS whisp_server_proc DESTINATION bin)
