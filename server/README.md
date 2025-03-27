# 项目名称
    Whisp

# 服务器请求处理架构设计
 Reactor 模式 + gRPC + epoll + 线程池

# C++ 服务器消息请求处理方案

## 1. 采用 Reactor 模式

- **主线程** 通过 `epoll` 监听客户端 `gRPC` 发送的请求。
- **请求分发** 采用 **表驱动法**，根据请求类型找到相应的处理函数。
- **线程池** 负责具体的请求处理，避免主线程阻塞，提高并发能力。

---

# 命名规范
类型	规则	示例
文件名	小写字母，下划线分隔	message_handler.cpp
函数名	小写字母，下划线分隔，动词开头	send_message()
变量名	小写字母，下划线分隔	user_name
常量	全大写字母，下划线分隔	MAX_BUFFER_SIZE
宏定义	全大写字母，下划线分隔	DEBUG_MODE
类名	驼峰命名法，首字母大写	MessageHandler
命名空间	全小写字母，下划线分隔	chat_server
模板参数	单个字母，大写	template <typename T>
指针/引用	指针以 ptr 结尾，引用以 ref 结尾	user_ptr, message_ref
枚举	类型名驼峰命名，枚举值全大写	enum class LogLevel
结构体	驼峰命名法，首字母大写	struct UserInfo


