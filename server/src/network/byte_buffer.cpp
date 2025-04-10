#include "byte_buffer.h"
#include "platform.h"
#include "w_sockets.h"
#include "net_callback.h"

using namespace w_network;
const char ByteBuffer::CLRF_[] = "\r\n";
const size_t ByteBuffer::cheap_prepend;
const size_t ByteBuffer::initial_size;


int32_t ByteBuffer::bb_read_fd(int fd, int* saved_errno)
{
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    const size_t writable = bb_bytes_writeable();
#ifdef WIN32
    struct iovec vec[2];

    vec[0].iov_base = _begin() + write_idx_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // when there is enough space in this ByteBuffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = w_sockets::socks_readv(fd, vec, iovcnt);
#else
    const int32_t n = w_sockets::socks_read(fd, extrabuf, sizeof(extrabuf));
#endif
    if (n <= 0)
    {
#ifdef WIN32
        * saved_errno = ::WSAGetLastError();
#else
        * saved_errno = errno;
#endif
    }
    else if (size_t(n) <= writable)
    {
#ifdef WIN32
        //Windows平台需要手动把接收到的数据加入ByteBuffer中，Linux平台已经在 struct iovec 中指定了缓冲区写入位置
        bb_append(extrabuf, n);
#else
    write_idx_ += n;
#endif
    }
    else
    {
#ifdef WIN32
        //Windows平台直接将所有的字节放入缓冲区去
        bb_append(extrabuf, n);
#else
        //Linux平台把剩下的字节补上去
        write_idx_ = buffer_.size();
        bb_append(extrabuf, n - writable);
#endif
    }
    // if (n == writable + sizeof extrabuf)
    // {
    //   goto line_30;
    // }
    return n;
}