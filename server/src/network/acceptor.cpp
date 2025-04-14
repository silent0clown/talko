
#include "acceptor.h"
#include "event_loop.h"
#include "inet_address.h"
#include "log/whisp_log.h"

using namespace w_network;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop),
    accept_socket_(w_sockets::socks_create_noblocking_or_die()),
    accept_channel_(loop, accept_socket_.fd()),
    listening_(false)
{
#ifndef WIN32
    idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
#endif

    accept_socket_.sock_set_reuse_addr(true);
    accept_socket_.sock_set_reuse_port(reuseport);
    accept_socket_.sock_bind_address(listenAddr);
    accept_channel_.set_read_callback(std::bind(&Acceptor::_handle_read, this));
}

Acceptor::~Acceptor()
{
    accept_channel_.disable_all();
    accept_channel_.remove();
#ifndef WIN32
    ::close(idle_fd_);
#endif
}

void Acceptor::listen()
{
    loop_->assert_in_loop_thread();
    listening_ = true;
    accept_socket_.sock_listen();
    accept_channel_.enable_reading();
}

void Acceptor::_handle_read()
{
    loop_->assert_in_loop_thread();
    InetAddress peerAddr;
    int connfd = accept_socket_.sock_accept(&peerAddr);
    if (connfd >= 0)
    {
        std::string hostport = peerAddr.inet_2_ipport();
        WHISP_LOG_DEBUG("Accepts of %s", hostport.c_str());
        //newConnectionCallback_实际指向TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
        if (new_conn_callback_)
        {
            new_conn_callback_(connfd, peerAddr);
        }
        else
        {
            w_sockets::socks_close(connfd);
        }
    }
    else
    {
        WHISP_LOG_SYSERROR("in Acceptor::handleRead");

#ifndef WIN32
        /*
        The special problem of accept()ing when you can't

        Many implementations of the POSIX accept function (for example, found in post-2004 Linux)
        have the peculiar behaviour of not removing a connection from the pending queue in all error cases.

        For example, larger servers often run out of file descriptors (because of resource limits),
        causing accept to fail with ENFILE but not rejecting the connection, leading to libev signalling
        readiness on the next iteration again (the connection still exists after all), and typically
        causing the program to loop at 100% CPU usage.

        Unfortunately, the set of errors that cause this issue differs between operating systems,
        there is usually little the app can do to remedy the situation, and no known thread-safe
        method of removing the connection to cope with overload is known (to me).

        One of the easiest ways to handle this situation is to just ignore it - when the program encounters
        an overload, it will just loop until the situation is over. While this is a form of busy waiting,
        no OS offers an event-based way to handle this situation, so it's the best one can do.

        A better way to handle the situation is to log any errors other than EAGAIN and EWOULDBLOCK,
        making sure not to flood the log with such messages, and continue as usual, which at least gives
        the user an idea of what could be wrong ("raise the ulimit!"). For extra points one could
        stop the ev_io watcher on the listening fd "for a while", which reduces CPU usage.

        If your program is single-threaded, then you could also keep a dummy file descriptor for overload
        situations (e.g. by opening /dev/null), and when you run into ENFILE or EMFILE, close it,
        run accept, close that fd, and create a new dummy fd. This will gracefully refuse clients under
        typical overload conditions.

        The last way to handle it is to simply log the error and exit, as is often done with malloc
        failures, but this results in an easy opportunity for a DoS attack.
        */
        if (errno == EMFILE)
        {
            ::close(idle_fd_);
            idle_fd_ = ::accept(accept_socket_.fd(), NULL, NULL);
            ::close(idle_fd_);
            idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
#endif
    }
}