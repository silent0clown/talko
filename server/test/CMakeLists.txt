# test/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(TalkoServerTests)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 GoogleTest 头文件路径
include_directories(${THIRD_PARTY_PATH}/googletest/include)

# 添加 GoogleTest 库路径
link_directories(${THIRD_PARTY_PATH}/googletest/lib)

# 添加主项目的头文件路径
include_directories(${ROOT_PATH}/src)

# 添加测试可执行文件
add_executable(TalkoTests
    test_main.cpp
    test_log.cpp
    test_db.cpp
    test_user_info.cpp
    test_thread_pool.cpp
)
if (0)
# 添三方库头文件路径
include_directories(${ROOT_PATH}/third_party/openssl-dist/include)
include_directories(${ROOT_PATH}/third_party/mysqlconn-dist/include)

# 添加库
add_library(crypto SHARED IMPORTED)
add_library(ssl SHARED IMPORTED)
add_library(mysqlcppconn8 SHARED IMPORTED)

# 添加库路径
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${ROOT_PATH}/third_party/openssl-dist/lib/libcrypto.a)
set_target_properties(ssl PROPERTIES IMPORTED_LOCATION ${ROOT_PATH}/third_party/openssl-dist/lib/libssl.a)
set_target_properties(mysqlcppconn8 PROPERTIES IMPORTED_LOCATION ${ROOT_PATH}/third_party/mysqlconn-dist/lib64/libmysqlcppconn8.so)
endif()

# 连接库文件
target_link_libraries(whisp_server_lib crypto ssl mysqlcppconn8)

# 链接主项目库和 GoogleTest 库
target_link_libraries(TalkoTests
    whisp_server_lib  # 主项目的库
    gtest
    gtest_main
    mysqlcppconn8
    crypto
    ssl
)

# 启用测试
enable_testing()
add_test(NAME TalkoTests COMMAND TalkoTests)

install(TARGETS TalkoTests DESTINATION test)