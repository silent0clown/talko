#pragma once

#include <string.h>
#include "platform.h"

namespace w_network
{
    class InetAddress {
    public:
        explicit InetAddress(uint16_t port = 0, bool loop_back = false);

        InetAddress(const std::string& ip, uint16_t port);

        InetAddress(const struct sockaddr_in& addr) : addr_(addr) {}

        std::string inet_2_ip() const;
        std::string inet_2_ipport() const;
        uint16_t inet_2_port() const;

        const struct sockaddr_in& inet_get_sockaddr() const { return addr_; }
        void inet_set_sockaddr(const struct sockaddr_in& addr) { addr_ = addr; }

        uint32_t inet_ip_netendian() const { return addr_.sin_addr.s_addr; }
        uint16_t inet_port_netendian() const { return addr_.sin_port; }

        static bool inet_resolve(const std::string& hostname, InetAddress* result);
    private:
        struct sockaddr_in addr_;
    };
}