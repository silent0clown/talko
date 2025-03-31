cmake_minimum_required(VERSION 3.15)
project(whisp_server_lib)

# 添加主项目源码
set(SRC_FILES
    config/parse_config.cpp
    #database/whisp_db.cpp
    #database/TalkMessageDAO.cpp
    database/db_user_info.cpp
    log/whisp_log.cpp
    #service/TalkConsumer.cpp
    #service/TalkMessage.cpp
    #service/TalkProducer.cpp
    #service/TalkServer.cpp
    #service/TalkUser.cpp
    #library/db_mysqlconn_pool.cpp
)

# 生成主项目库
add_library(whisp_server_lib STATIC ${SRC_FILES})

# 包含头文件路径
target_include_directories(whisp_server_lib PUBLIC 
    ${ROOT_PATH}/src
    ${ROOT_PATH}/src/config
    ${ROOT_PATH}/src/database
    ${ROOT_PATH}/src/log
    ${ROOT_PATH}/src/service
    ${ROOT_PATH}/src/library
)

# 连接库文件
# target_link_libraries(whisp_server_lib crypto ssl mysqlcppconn8 yaml-cpp)


add_library(resolv SHARED IMPORTED)
set_target_properties(resolv PROPERTIES IMPORTED_LOCATION /usr/lib/x86_64-linux-gnu/libresolv.so)
target_link_libraries(whisp_server_lib mysqlcppconn8 ssl crypto dl z resolv yaml-cpp)

# 生成可执行文件
add_executable(whisp_server_proc main.cpp)

# 链接 src 目录生成的库
target_link_libraries(whisp_server_proc PRIVATE whisp_server_lib)