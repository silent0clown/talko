#pragma once

// #include <stdint.h>
#include "platform.h" // endin.h stdint.h

namespace w_network
{
	namespace w_sockets
	{
		inline uint64_t host_2_network64(uint64_t host64)
		{
#ifdef WIN32
            return htonll(host64);
#else
			return htobe64(host64);
#endif
		}

		inline uint32_t host_2_network32(uint32_t host32)
		{
#ifdef WIN32
            return htonl(host32);
#else
            return htobe32(host32);
#endif
		}

		inline uint16_t host_2_network16(uint16_t host16)
		{
#ifdef WIN32		
			return htons(host16);
#else
            return htobe16(host16);
#endif
		}

		inline uint64_t network_2_host64(uint64_t net64)
		{
#ifdef WIN32
            return ntohll(net64);
#else
			return be64toh(net64);
#endif
		}

		inline uint32_t network_2_host32(uint32_t net32)
		{
#ifdef WIN32
            return ntohl(net32);
#else
			return be32toh(net32);
#endif
		}

		inline uint16_t network_2_host16(uint16_t net16)
		{
#ifdef WIN32
			return ntohs(net16);
#else
            return be16toh(net16);
#endif
		}
	}// end namespace sockets
}// end namespace net
