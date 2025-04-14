#include "tcp_connect.h"
#include "event_loop.h"
#include "log/whisp_log.h"
#include "channel.h"

using namespace w_network;

void w_network::default_conn_callback(const TcpConnectionPtr& conn) 
{
    WHISP_LOG_DEBUG("%s -> is %s",
        conn->local_address().inet_2_ipport().c_str(),
        std::to_string(conn->peer_address().inet_2_port()).c_str(),
        (conn->connected() ? "UP" : "DOWN"));
}

void w_network::default_msg_callback(const TcpConnectionPtr& conn, ByteBuffer* buffer, Timestamp recv_time) 
{
    buffer->bb_retrieve_all();
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop_(loop),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    local_addr_(localAddr),
    peer_addr_(peerAddr),
    high_water_mark_(64 * 1024 * 1024)
{
    channel_->set_read_callback(std::bind(&TcpConnection::_handle_read, this, std::placeholders::_1));
    channel_->set_write_callback(std::bind(&TcpConnection::_handle_write, this));
    channel_->set_close_callback(std::bind(&TcpConnection::_handle_close, this));
    channel_->set_error_callback(std::bind(&TcpConnection::_handle_error, this));
    WHISP_LOG_DEBUG("TcpConnection::ctor[%s] at 0x%x fd=%d", name_.c_str(), this, sockfd);
    socket_->sock_set_keepalive(true);
}

TcpConnection::~TcpConnection()
{
    WHISP_LOG_DEBUG("TcpConnection::dtor[%s] at 0x%x fd=%d state=%s",
        name_.c_str(), this, channel_->fd(), _state_2_string());
    //assert(state_ == kDisconnected);
}

void TcpConnection::send(const void* data, int len)
{
    if (state_ == kConnected)
    {
        if (loop_->is_in_loop_thread())
        {
            _send_in_loop(data, len);
        }
        else
        {
            std::string message(static_cast<const char*>(data), len);
            loop_->run_in_loop(
                std::bind(static_cast<void (TcpConnection::*)(const std::string&)>(&TcpConnection::_send_in_loop),
                    this,     // FIXME
                    message));
        }
    }
}

void TcpConnection::send(const std::string& message)
{
    if (state_ == kConnected)
    {
        if (loop_->is_in_loop_thread())
        {
            _send_in_loop(message);
        }
        else
        {
            loop_->run_in_loop(
                std::bind(static_cast<void (TcpConnection::*)(const std::string&)>(&TcpConnection::_send_in_loop),
                    this,     // FIXME
                    message));
            //std::forward<std::string>(message)));
        }
    }
}

// FIXME efficiency!!!
void TcpConnection::send(ByteBuffer* buf)
{
    if (state_ == kConnected)
    {
        if (loop_->is_in_loop_thread())
        {
            _send_in_loop(buf->bb_peek(), buf->bb_bytes_readable());
            buf->bb_retrieve_all();
        }
        else
        {
            loop_->run_in_loop(
                std::bind(static_cast<void (TcpConnection::*)(const std::string&)>(&TcpConnection::_send_in_loop),
                    this,     // FIXME
                    buf->bb_retrieve_all_as_string()));
            //std::forward<std::string>(message)));
        }
    }
}

void TcpConnection::_send_in_loop(const std::string& message)
{
    _send_in_loop(message.c_str(), message.size());
}

void TcpConnection::_send_in_loop(const void* data, size_t len)
{
    loop_->assert_in_loop_thread();
    int32_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
        WHISP_LOG_WARN("disconnected, give up writing");
        return;
    }
    // if no thing in output queue, try writing directly
    if (!channel_->is_writing() && output_buffer_.bb_bytes_readable() == 0)
    {
        nwrote = w_sockets::socks_write(channel_->fd(), data, len);
        //TODO: 打印threadid用于调试，后面去掉
        //std::stringstream ss;
        //ss << std::this_thread::get_id();
        //LOGI << "send data in threadID = " << ss;

        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && write_complete_callback_)
            {
                loop_->queue_in_loop(std::bind(write_complete_callback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                WHISP_LOG_SYSERROR("TcpConnection::_send_in_loop");
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    //assert(remaining <= len);
    if (remaining > len)
        return;

    if (!faultError && remaining > 0)
    {
        size_t oldLen = output_buffer_.bb_bytes_readable();
        if (oldLen + remaining >= high_water_mark_
            && oldLen < high_water_mark_
            && high_water_mark_callback_)
        {
            loop_->queue_in_loop(std::bind(high_water_mark_callback_, shared_from_this(), oldLen + remaining));
        }
        output_buffer_.bb_append(static_cast<const char*>(data) + nwrote, remaining);
        if (!channel_->is_writing())
        {
            channel_->enable_writing();
        }
    }
}

void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (state_ == kConnected)
    {
        _set_state(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->run_in_loop(std::bind(&TcpConnection::_shutdown_in_loop, this));
    }
}

void TcpConnection::_shutdown_in_loop()
{
    loop_->assert_in_loop_thread();
    if (!channel_->is_writing())
    {
        // we are not writing
        socket_->sock_shutdown_write();
    }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     _set_state(kDisconnecting);
//     loop_->run_in_loop(boost::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assert_in_loop_thread();
//   if (!channel_->is_writing())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::_force_close_in_loop));
// }

void TcpConnection::force_close()
{
    // FIXME: use compare and swap
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        _set_state(kDisconnecting);
        loop_->queue_in_loop(std::bind(&TcpConnection::_force_close_in_loop, shared_from_this()));
    }
}


void TcpConnection::_force_close_in_loop()
{
    loop_->assert_in_loop_thread();
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        // as if we received 0 byte in handle_read();
        _handle_close();
    }
}

const char* TcpConnection::_state_2_string() const
{
    switch (state_)
    {
    case kDisconnected:
        return "kDisconnected";
    case kConnecting:
        return "kConnecting";
    case kConnected:
        return "kConnected";
    case kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknown state";
    }
}

void TcpConnection::set_tcp_nodelay(bool on)
{
    socket_->sock_set_tcp_nodelay(on);
}

void TcpConnection::conn_established()
{
    loop_->assert_in_loop_thread();
    if (state_ != kConnecting)
    {
        //一定不能走这个分支
        return;
    }

    _set_state(kConnected);

    //假如正在执行这行代码时，对端关闭了连接
    if (!channel_->enable_reading())
    {
        WHISP_LOG_ERROR("enable_reading failed.");
        //_set_state(kDisconnected);
        _handle_close();
        return;
    }

    //connectionCallback_指向void XXServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
    conn_callback_(shared_from_this());
}

void TcpConnection::conn_destroyed()
{
    loop_->assert_in_loop_thread();
    if (state_ == kConnected)
    {
        _set_state(kDisconnected);
        channel_->disable_all();

        conn_callback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::_handle_read(Timestamp receiveTime)
{
    loop_->assert_in_loop_thread();
    int savedErrno = 0;
    int32_t n = input_buffer_.bb_read_fd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        //messageCallback_指向CTcpSession::OnRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receiveTime)
        msg_callback_(shared_from_this(), &input_buffer_, receiveTime);
    }
    else if (n == 0)
    {
        _handle_close();
    }
    else
    {
        errno = savedErrno;
        WHISP_LOG_SYSERROR("TcpConnection::handle_read");
        _handle_error();
    }
}

void TcpConnection::_handle_write()
{
    loop_->assert_in_loop_thread();
    if (channel_->is_writing())
    {
        int32_t n = w_sockets::socks_write(channel_->fd(), output_buffer_.bb_peek(), output_buffer_.bb_bytes_readable());
        if (n > 0)
        {
            output_buffer_.bb_retrieve(n);
            if (output_buffer_.bb_bytes_readable() == 0)
            {
                channel_->disable_writing();
                if (write_complete_callback_)
                {
                    loop_->queue_in_loop(std::bind(write_complete_callback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    _shutdown_in_loop();
                }
            }
        }
        else
        {
            WHISP_LOG_SYSERROR("TcpConnection::_handle_write");
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
            //added by zhangyl 2019.05.06
            _handle_close();
        }
    }
    else
    {
        WHISP_LOG_DEBUG("Connection fd = %d  is down, no more writing", channel_->fd());
    }
}

void TcpConnection::_handle_close()
{
    //在Linux上当一个链接出了问题，会同时触发handleError和handleClose
    //为了避免重复关闭链接，这里判断下当前状态
    //已经关闭了，直接返回
    if (state_ == kDisconnected)
        return;

    loop_->assert_in_loop_thread();
    WHISP_LOG_DEBUG("fd = %d  state = %s", channel_->fd(), _state_2_string());
    //assert(state_ == kConnected || state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    _set_state(kDisconnected);
    channel_->disable_all();

    TcpConnectionPtr guardThis(shared_from_this());
    conn_callback_(guardThis);
    // must be the last line
    close_callback_(guardThis);

    //只处理业务上的关闭，真正的socket fd在TcpConnection析构函数中关闭
    //if (socket_)
    //{
    //    w_sockets::close(socket_->fd());
    //}
}

void TcpConnection::_handle_error()
{
    int err = w_sockets::socks_get_error(channel_->fd());
    WHISP_LOG_ERROR("TcpConnection::%s _handle_error [%d] - SO_ERROR = %s", name_.c_str(), err, strerror(err));

    //调用handleClose()关闭连接，回收Channel和fd
    _handle_close();
}