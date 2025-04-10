/* TCP服务器类， 基于Reactor模式 */
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <atomic>
#include <map>
#include <memory>
#include <string>
// #include "TcpConnection.h"

namespace network {
    class Acceptor;
    class EventLoop;
    class EventLoopThreadPool;

    class TcpServer {
    public:
      typedef std::function<void(EventLoop*)> thread_init_callback;
      enum PORT_OPTION {
        NO_RE_USE_PORT,
        RE_USE_PORT,
      };
      TcpServer(EventLoop* loop, const InetAddress& listend_addr, const std::string& name_arg, PORT_OPTION option = RE_USE_PORT);
      ~TcpServer();
    
    private:
      EventLoop* loop_;
      const std::string host_port_;
      const std::string name_;
      std::unique_ptr<Acceptor> acceptor_;
      std::unique_ptr<EventLoopThreadPool> eventloop_thread_pool_;
      ConnectionCallback   connect_callback_;
      MessageCallback      msg_callback_;
      WriteCompleteCallback write_complete_callback_;
      thread_init_callback thread_init_callback_;
      std::atomic<int> running_flag_;
      int next_conn_id_;
      

    };
}

#endif // TCP_SERVER_H