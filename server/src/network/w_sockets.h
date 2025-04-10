#pragma once

#include <stdint.h>
#include "platform.h"

struct tcp_info;

namespace w_network
{
    class InetAddress;

    class Socket {
    public:
        explicit Socket(int sockfd) : sockfd_(sockfd) {}
        ~Socket();

        SOCKET fd() const { return sockfd_; }
        void sock_bind_address(const InetAddress& local_addr);
        void sock_listen();
        int sock_accept(InetAddress* peer_addr);
        void sock_shutdown_write();
        void sock_set_tcp_nodelay(bool on);
        void sock_set_reuse_addr(bool on);
        void sock_set_reuse_port(bool on);
        void sock_set_keepalive(bool on);

    private:
        const SOCKET sockfd_;
    };

    namespace w_sockets 
    {
        SOCKET socks_create_or_die();
        SOCKET socks_create_noblocking_or_die();

        void socks_set_noblocking_and_close_exec(SOCKET sockfd);

        void socks_set_reuse_addr(SOCKET sockfd, bool on);
        void socks_set_reuse_port(SOCKET sockfd, bool on);

        SOCKET socks_connect(SOCKET sockfd, const struct sockaddr_in& addr);
        void socks_bind_or_die(SOCKET sockfd, const struct sockaddr_in& addr);
        void socks_listen_or_die(SOCKET sockfd);
        SOCKET socks_accept(SOCKET sockfd, struct sockaddr_in* addr);
        int32_t socks_read(SOCKET sockfd, void* buf, int32_t count);
#ifndef WIN32
        ssize_t socks_readv(SOCKET sockfd, const struct iovec* iov, int iovcnt);
#endif
        int32_t socks_write(SOCKET sockfd, const void* buf, int32_t count);
        void socks_close(SOCKET sockfd);
        void socks_shutdown_write(SOCKET sockfd);

        void socks_2_ipport(char* buf, size_t size, const struct sockaddr_in& addr);
        void socks_2_ip(char* buf, size_t size, const struct sockaddr_in& addr);
        void socks_from_ipport(const char* ip, uint16_t port, struct sockaddr_in* addr);

        int socks_get_error(SOCKET sockfd);

        const struct sockaddr* socks_addr_cast(const struct sockaddr_in* addr);
        struct sockaddr* socks_addr_cast(struct sockaddr_in* addr);
        const struct sockaddr_in* socks_addr_in_cast(const struct sockaddr* addr);
        struct sockaddr_in* socks_addr_in_cast(struct sockaddr* addr);

        struct sockaddr_in socks_get_local_addr(SOCKET sockfd);
        struct sockaddr_in socks_get_peer_addr(SOCKET sockfd);
        bool socks_is_selfconn(SOCKET sockfd);
    }
}
