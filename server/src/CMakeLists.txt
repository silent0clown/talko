cmake_minimum_required(VERSION 3.15)
project(TalkoServerSrc)

# 添加主项目源码
set(SRC_FILES
    config/TalkConfig.cpp
    database/TalkDB.cpp
    database/TalkMessageDAO.cpp
    database/TalkUserDAO.cpp
    log/TalkLog.cpp
    service/TalkConsumer.cpp
    service/TalkMessage.cpp
    service/TalkProducer.cpp
    service/TalkServer.cpp
    service/TalkUser.cpp
)

# 生成主项目库
add_library(TalkoServerLib STATIC ${SRC_FILES})



# 包含头文件路径
target_include_directories(TalkoServerLib PUBLIC 
    ${ROOT_PATH}/src
    ${ROOT_PATH}/src/config
    ${ROOT_PATH}/src/database
    ${ROOT_PATH}/src/log
    ${ROOT_PATH}/src/service
)

# 生成可执行文件
add_executable(TalkoServer main.cpp)

# 链接 src 目录生成的库
target_link_libraries(TalkoServer PRIVATE TalkoServerLib)