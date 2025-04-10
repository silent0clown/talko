#include "w_sockets.h"

#include <stdio.h> // snprintf
#include <string.h>

#include "log/whisp_log.h"
#include "inet_address.h"
#include "inet_endian.h"

using namespace w_network;

Socket::~Socket()
{
    w_sockets::socks_close(sockfd_);
}

void Socket::sock_bind_address(const InetAddress& addr)
{
    w_sockets::socks_bind_or_die(sockfd_, addr.inet_get_sockaddr());
}

void Socket::sock_listen()
{
    w_sockets::socks_listen_or_die(sockfd_);
}

int Socket::sock_accept(InetAddress* peer_addr)
{
    struct sockaddr_in addr;
    memset(&addr, 0 ,sizeof(addr));
    int connfd = w_sockets::socks_accept(sockfd_, &addr);
    if (connfd >= 0) {
        peer_addr->inet_set_sockaddr(addr);
    }
    
    return connfd;
}

void Socket::sock_shutdown_write()
{
    w_sockets::socks_shutdown_write(sockfd_);
}

void Socket::sock_set_tcp_nodelay(bool on)
{
    int optval = on ? 1 : 0;
#ifdef WIN32
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
#else
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
#endif
    // FIXME CHECK    
}

void Socket::sock_set_reuse_addr(bool on)
{
    w_sockets::socks_set_reuse_addr(sockfd_, on);
}

void Socket::sock_set_reuse_port(bool on)
{
    w_sockets::socks_set_reuse_port(sockfd_, on);
}

void Socket::sock_set_keepalive(bool on)
{
    #ifdef WIN32
    //TODO: 补全Windows的写法
#else
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
#endif
    // FIXME CHECK
}


const struct sockaddr* w_sockets::socks_addr_cast(const struct sockaddr_in* addr)
{
    return static_cast<const struct sockaddr*>((const void*)(addr));
}

struct sockaddr* w_sockets::socks_addr_cast(struct sockaddr_in* addr)
{
    return static_cast<struct sockaddr*>((void*)(addr));
}

const struct sockaddr_in* w_sockets::socks_addr_in_cast(const struct sockaddr* addr)
{
    return static_cast<const struct sockaddr_in*>((const void*)(addr));
}

struct sockaddr_in* w_sockets::socks_addr_in_cast(struct sockaddr* addr)
{
    return static_cast<struct sockaddr_in*>((void*)(addr));
}

/***************************** w_sockets begin ***************************/
SOCKET w_sockets::socks_create_or_die()
{
#ifdef WIN32
    SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET)
    {
        WHISP_LOG_FALTAL("w_sockets::socks_create_or_die");
    }
#else
    SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET)
    {
        WHISP_LOG_FALTAL("w_sockets::socks_create_or_die");
    }
#endif

    return sockfd;    
}

SOCKET w_sockets::socks_create_noblocking_or_die()
{
#ifdef WIN32
    SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET)
    {
        WHISP_LOG_FALTAL("w_sockets::socks_create_noblocking_or_die");
    }
#else
    SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET)
    {
        WHISP_LOG_FALTAL("w_sockets::socks_create_noblocking_or_die");
    }
#endif

    socks_set_noblocking_and_close_exec(sockfd);
    
    return sockfd;
}

void w_sockets::socks_set_noblocking_and_close_exec(SOCKET sockfd)
{
#ifdef WIN32
    //将socket设置成非阻塞的
    unsigned long on = 1;
    ::ioctlsocket(sockfd, FIONBIO, &on);
#else
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    // FIXME check

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
    // FIXME check

    (void)ret;
#endif       
}


void w_sockets::socks_bind_or_die(SOCKET sockfd, const struct sockaddr_in& addr)
{
    int ret = ::bind(sockfd, socks_addr_cast(&addr), static_cast<socklen_t>(sizeof(addr)));
    if (ret == SOCKET_ERROR) {
        WHISP_LOG_FALTAL("w_sockets::socks_bind_or_die");
    }
}

void w_sockets::socks_listen_or_die(SOCKET sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if (SOCKET_ERROR == ret) {
        WHISP_LOG_FALTAL("w_sockets::socks_listen_or_die");
    }
}

SOCKET w_sockets::socks_accept(SOCKET sockfd, struct sockaddr_in* addr)
{
    socklen_t addrlen = static_cast<socklen_t>(sizeof * addr);
#ifdef WIN32
    SOCKET connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else  
    SOCKET connfd = ::accept4(sockfd, socks_addr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connfd == SOCKET_ERROR)
    {
#ifdef WIN32
        int savedErrno = ::WSAGetLastError();
        WHISP_LOG_SYSERROR("w_sockets::socks_accept");
        if (savedErrno != WSAEWOULDBLOCK)
        WHISP_LOG_FALTAL("unexpected error of ::accept %d", savedErrno);
#else
        int savedErrno = errno;
        WHISP_LOG_SYSERROR("w_sockets::socks_accept");
        switch (savedErrno)
        {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO: // ???
        case EPERM:
        case EMFILE: // per-process lmit of open file desctiptor ???
            // expected errors
            errno = savedErrno;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            // unexpected errors
            WHISP_LOG_FALTAL("unexpected error of ::accept %d", savedErrno);
            break;
        default:
            WHISP_LOG_FALTAL("unknown error of ::accept %d", savedErrno);
            break;
        }

#endif
    }

    return connfd;    
}


void w_sockets::socks_set_reuse_addr(SOCKET sockfd, bool on)
{
    int optval = on ? 1 : 0;
#ifdef WIN32
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
#else
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
#endif
    // FIXME CHECK
}

void w_sockets::socks_set_reuse_port(SOCKET sockfd, bool on)
{
    //Windows 系统没有 SO_REUSEPORT 选项
#ifndef WIN32
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        WHISP_LOG_SYSERROR("SO_REUSEPORT failed.");
    }
#endif
}

SOCKET w_sockets::socks_connect(SOCKET sockfd, const struct sockaddr_in& addr)
{
    return ::connect(sockfd, socks_addr_cast(&addr), static_cast<socklen_t>(sizeof addr));
}

int32_t w_sockets::socks_read(SOCKET sockfd, void* buf, int32_t count)
{
#ifdef WIN32
    return ::recv(sockfd, (char*)buf, count, 0);
#else
    return ::read(sockfd, buf, count);
#endif
}

#ifndef WIN32
ssize_t w_sockets::socks_readv(SOCKET sockfd, const struct iovec* iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}
#endif

int32_t w_sockets::socks_write(SOCKET sockfd, const void* buf, int32_t count)
{
#ifdef WIN32
    return ::send(sockfd, (const char*)buf, count, 0);
#else
    return ::write(sockfd, buf, count);
#endif

}

void w_sockets::socks_close(SOCKET sockfd)
{
#ifdef WIN32   
    if (::closesocket(sockfd) < 0)
#else
    if (::close(sockfd) < 0)
#endif
    {
        WHISP_LOG_SYSERROR("w_sockets::close, fd=%d, errno=%d, errorinfo=%s", sockfd, errno, strerror(errno));
    }
}

void w_sockets::socks_shutdown_write(SOCKET sockfd)
{
#ifdef WIN32
    if (::shutdown(sockfd, SD_SEND) < 0)
#else
    if (::shutdown(sockfd, SHUT_WR) < 0)
#endif        
    {
        WHISP_LOG_SYSERROR("w_sockets::shutdownWrite");
    }
}

void w_sockets::socks_2_ipport(char* buf, size_t size, const struct sockaddr_in& addr)
{
    //if (size >= sizeof(struct sockaddr_in))
    //    return;

    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
    size_t end = ::strlen(buf);
    uint16_t port = w_sockets::network_2_host16(addr.sin_port);
    //if (size > end)
    //    return;

    snprintf(buf + end, size - end, ":%u", port);
}

void w_sockets::socks_2_ip(char* buf, size_t size, const struct sockaddr_in& addr)
{
    if (size >= sizeof(struct sockaddr_in))
        return;

    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
}

void w_sockets::socks_from_ipport(const char* ip, uint16_t port, struct sockaddr_in* addr)
{
    addr->sin_family = AF_INET;
    //TODO: 校验下写的对不对
#ifdef WIN32
    addr->sin_port = htons(port);
#else
    addr->sin_port = htobe16(port);
#endif
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    {
        WHISP_LOG_SYSERROR("w_sockets::fromIpPort");
    }
}

int w_sockets::socks_get_error(SOCKET sockfd)
{
    int optval;
#ifdef WIN32
    int optvallen = sizeof(optval);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&optval, &optvallen) < 0)
        return ::WSAGetLastError();
#else
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        return errno;
#endif
    return optval;
}

struct sockaddr_in w_sockets::socks_get_local_addr(SOCKET sockfd)
{
    struct sockaddr_in localaddr = { 0 };
    memset(&localaddr, 0, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    ::getsockname(sockfd, socks_addr_cast(&localaddr), &addrlen);
    //if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
    //{
    //  LOG_SYSERR << "w_sockets::getLocalAddr";
    //  return 
    //}
    return localaddr;
}

struct sockaddr_in w_sockets::socks_get_peer_addr(SOCKET sockfd)
{
    struct sockaddr_in peeraddr = { 0 };
    memset(&peeraddr, 0, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    ::getpeername(sockfd, socks_addr_cast(&peeraddr), &addrlen);
    //if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
    //{
    //  LOG_SYSERR << "w_sockets::getPeerAddr";
    //}
    return peeraddr;
}

bool w_sockets::socks_is_selfconn(SOCKET sockfd)
{
    struct sockaddr_in localaddr = socks_get_local_addr(sockfd);
    struct sockaddr_in peeraddr = socks_get_peer_addr(sockfd);
    return localaddr.sin_port == peeraddr.sin_port && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}

/***************************** w_sockets end ***************************/