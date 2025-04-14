#include "tcp_server.h"
#include "tcp_connect.h"
#include "acceptor.h"
#include "event_loop.h"
#include "event_loop_threadpool.h"

using namespace w_network;

TcpServer::TcpServer(EventLoop* loop,
    const InetAddress& listenAddr,
    const std::string& nameArg,
    Option option)
    : loop_(loop),
    host_port_(listenAddr.inet_2_ipport()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    //threadPool_(new EventLoopThreadPool(loop, name_)),
    conn_callback_(default_conn_callback),
    msg_callback_(default_msg_callback),
    started_(0),
    next_conn_id_(1)
{
    acceptor_->set_new_conn_callback(std::bind(&TcpServer::new_conn, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    loop_->assert_in_loop_thread();
    WHISP_LOG_DEBUG("TcpServer::~TcpServer [%s] destructing", name_.c_str());

    stop();
}

//void TcpServer::setThreadNum(int numThreads)
//{
//  assert(0 <= numThreads);
//  threadPool_->setThreadNum(numThreads);
//}

void TcpServer::start(int workerThreadCount/* = 4*/)
{
    if (started_ == 0)
    {
        event_loop_threadpool_.reset(new EventLoopThreadPool());
        event_loop_threadpool_->init(loop_, workerThreadCount);
        event_loop_threadpool_->start();

        //threadPool_->start(threadInitCallback_);
        //assert(!acceptor_->listenning());
        loop_->run_in_loop(std::bind(&Acceptor::listen, acceptor_.get()));
        started_ = 1;
    }
}

void TcpServer::stop()
{
    if (started_ == 0)
        return;

    for (ConnectionMap::iterator it = conn_map_.begin(); it != conn_map_.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset();
        conn->get_loop()->run_in_loop(std::bind(&TcpConnection::conn_destroyed, conn));
        conn.reset();
    }

    event_loop_threadpool_->stop();

    started_ = 0;
}

void TcpServer::new_conn(int sockfd, const InetAddress& peerAddr)
{
    loop_->assert_in_loop_thread();
    EventLoop* ioLoop = event_loop_threadpool_->get_next_loop();
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", host_port_.c_str(), next_conn_id_);
    ++next_conn_id_;
    std::string connName = name_ + buf;

    WHISP_LOG_DEBUG("TcpServer::newConnection [%s] - new connection [%s] from %s", name_.c_str(), connName.c_str(), peerAddr.inet_2_ipport().c_str());

    InetAddress localAddr(w_sockets::socks_get_local_addr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    conn_map_[connName] = conn;
    conn->set_conn_callback(conn_callback_);
    conn->set_msg_callback(msg_callback_);
    conn->set_write_complete_callback(write_complete_callback_);
    conn->set_close_callback(std::bind(&TcpServer::remove_conn, this, std::placeholders::_1)); // FIXME: unsafe
    //该线程分离完io事件后，立即调用TcpConnection::conn_established
    ioLoop->run_in_loop(std::bind(&TcpConnection::conn_established, conn));
}

void TcpServer::remove_conn(const TcpConnectionPtr& conn)
{
    // FIXME: unsafe
    loop_->run_in_loop(std::bind(&TcpServer::remove_conn_in_loop, this, conn));
}

void TcpServer::remove_conn_in_loop(const TcpConnectionPtr& conn)
{
    loop_->assert_in_loop_thread();
    WHISP_LOG_DEBUG("TcpServer::remove_conn_in_loop [%s] - connection %s", name_.c_str(), conn->name().c_str());
    size_t n = conn_map_.erase(conn->name());
    //(void)n;
    //assert(n == 1);
    if (n != 1)
    {
        //出现这种情况，是TcpConneaction对象在创建过程中，对方就断开连接了。
        WHISP_LOG_DEBUG("TcpServer::remove_conn_in_loop [%s] - connection %s, connection does not exist.", name_.c_str(), conn->name().c_str());
        return;
    }

    EventLoop* ioLoop = conn->get_loop();
    ioLoop->queue_in_loop(std::bind(&TcpConnection::conn_destroyed, conn));
}