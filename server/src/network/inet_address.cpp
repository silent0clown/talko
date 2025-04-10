#include "inet_address.h"
#include <string.h>
#include "log/whisp_log.h"
#include "inet_endian.h"
#include "w_sockets.h"

// #include "Endian.h"

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

using namespace w_network;


InetAddress::InetAddress(uint16_t port, bool loopbackOnly/* = false*/)
{
    memset(&addr_, 0, sizeof addr_);
    addr_.sin_family = AF_INET;
    in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
    addr_.sin_addr.s_addr = w_sockets::host_2_network32(ip);
    addr_.sin_port = w_sockets::host_2_network16(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port)
{
    memset(&addr_, 0, sizeof addr_);
    w_sockets::socks_from_ipport(ip.c_str(), port, &addr_);
}

std::string InetAddress::inet_2_ipport() const
{
    char buf[32];
    w_sockets::socks_2_ipport(buf, sizeof buf, addr_);
    return buf;
}

std::string InetAddress::inet_2_ip() const
{
    char buf[32];
    w_sockets::socks_2_ip(buf, sizeof buf, addr_);
    return buf;
}

uint16_t InetAddress::inet_2_port() const
{
    return w_sockets::network_2_host16(addr_.sin_port);
}

static thread_local char t_resolveBuffer[64 * 1024];

bool InetAddress::inet_resolve(const std::string& hostname, InetAddress* out)
{
    //assert(out != NULL);
    struct hostent hent;
    struct hostent* he = NULL;
    int herrno = 0;
    memset(&hent, 0, sizeof(hent));

#ifndef WIN32
    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
    if (ret == 0 && he != NULL)
    {
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        return true;
    }

    if (ret)
    {
        WHISP_LOG_SYSERROR("InetAddress::inet_resolve");
    }

#endif
    //TODO: Windows上重新实现一下
    return false;
}
