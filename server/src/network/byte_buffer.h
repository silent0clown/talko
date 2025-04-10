#pragma once

#include "platform.h"
#include <algorithm>
#include <string>
#include <string.h>     // strlen()
#include "w_sockets.h"
#include "inet_endian.h"

namespace w_network
{
    /// 模仿 org.jboss.netty.buffer.ChannelByteBuffer
    /// +-------------------+------------------+------------------+
    /// | prependable bytes |  readable bytes  |  writable bytes  |
    /// |                   |     (CONTENT)    |                  |
    /// +-------------------+------------------+------------------+
    /// |                   |                  |                  |
    /// 0      <=      readerIndex   <=   writerIndex    <=     size
    class ByteBuffer {
    public:
        static const size_t cheap_prepend = 8;
        static const size_t initial_size  = 1024;

        explicit ByteBuffer(size_t size = initial_size) : 
            buffer_(cheap_prepend + initial_size),
            read_idx_(cheap_prepend),
            write_idx_(cheap_prepend) 
        {
        
        }

        void bb_swap(ByteBuffer& rhs)
        {
            buffer_.swap(rhs.buffer_);
            std::swap(read_idx_, rhs.read_idx_);
            std::swap(write_idx_, rhs.write_idx_);
        }

        size_t bb_bytes_readable() const
        {
            return write_idx_ - read_idx_;
        }

        size_t bb_bytes_writeable() const
        {
            return buffer_.size() - write_idx_;
        }

        size_t bb_bytes_prependable() const
        {
            return read_idx_;
        }

        const char* bb_peek() const
        {
            return _begin() + read_idx_;
        }

        const char* bb_find_string(const char* target) const
        {
            const char* found = std::search(bb_peek(), bb_write_beginning(), target, target + strlen(target));
            return found == bb_write_beginning() ? nullptr : found;
        }

        const char* bb_find_CLRF() const 
        {
            const char* crlf = std::search(bb_peek(), bb_write_beginning(), CLRF_, CLRF_ + 2);
            return crlf == bb_write_beginning() ? nullptr : crlf;            
        }

        const char* bb_find_CLRF(const char* start) const
        {
            if (bb_peek() > start)
                return nullptr;

            if (start > bb_write_beginning())
                return nullptr;

            // FIXME: replace with memmem()?
            const char* crlf = std::search(start, bb_write_beginning(), CLRF_, CLRF_ + 2);
            return crlf == bb_write_beginning() ? nullptr : crlf;
        }        

        const char* bb_find_EOL() const
        {
            const void* eol = memchr(bb_peek(), '\n', bb_bytes_readable());
            return static_cast<const char*>(eol);
        }

        const char* bb_findEOL(const char* start) const
        {
            if (bb_peek() > start)
                return nullptr;

            if (start > bb_write_beginning())
                return nullptr;

            const void* eol = memchr(start, '\n', bb_write_beginning() - start);
            return static_cast<const char*>(eol);
        }

        bool bb_retrieve(size_t len)
        {
            if (len > bb_bytes_readable())
                return false;

            if (len < bb_bytes_readable())
            {
                read_idx_ += len;
            }
            else
            {
                bb_retrieve_all();
            }

            return true;
        }

        bool bb_retrieve_until(const char* end)
        {
            if (bb_peek() > end)
                return false;

            if (end > bb_write_beginning())
                return false;

            bb_retrieve(end - bb_peek());

            return true;
        }

        void bb_retrieve_int64()
        {
            bb_retrieve(sizeof(int64_t));
        }

        void bb_retrieve_int32()
        {
            bb_retrieve(sizeof(int32_t));
        }

        void bb_retrieve_int16()
        {
            bb_retrieve(sizeof(int16_t));
        }

        void bb_retrieve_int8()
        {
            bb_retrieve(sizeof(int8_t));
        }

        void bb_retrieve_all()
        {
            read_idx_ = cheap_prepend;
            write_idx_ = cheap_prepend;
        }        

        std::string bb_retrieve_all_as_string()
        {
            return bb_retrieve_as_string(bb_bytes_readable());;
        }

        std::string bb_retrieve_as_string(size_t len)
        {
            if (len > bb_bytes_readable())
                return "";

            std::string result(bb_peek(), len);
            bb_retrieve(len);
            return result;
        }

        std::string bb_2_string_piece() const
        {
            return std::string(bb_peek(), static_cast<int>(bb_bytes_readable()));
        }

        void bb_append(const std::string& str)
        {
            bb_append(str.c_str(), str.size());
        }

        void bb_append(const char* /*restrict*/ data, size_t len)
        {
            //其实相当于把已有数据往前挪动
            bb_bytes_check_writable(len);
            std::copy(data, data + len, bb_bytes_writeable());
            bb_bytes_written(len);
        }

        void bb_append(const void* /*restrict*/ data, size_t len)
        {
            bb_append(static_cast<const char*>(data), len);
        }

        bool bb_bytes_check_writable(size_t len)
        {
            //剩下的可写空间如果小于需要的空间len，则增加len长度个空间
            if (bb_bytes_writeable() < len)
            {
                _make_zoom(len);
            }

            return true;
        }        

        char* bb_write_beginning()
        {
            return write_idx_ + _begin();
        }

        const char* bb_write_beginning() const
        {
            return write_idx_ + _begin();
        }

        bool bb_bytes_written(size_t len)
        {
            if (len > bb_bytes_writeable())
                return false;

            write_idx_ += len;
            return true;
        }

        bool bb_bytes_unwrite(size_t len)
        {
            if (len > bb_bytes_readable()) {
                return false;
            }

            write_idx_ -= len;
            return true;
        }

        void bb_append_int64(int64_t x)
        {
            int64_t be64 = w_sockets::host_2_network64(x);
            bb_append(&be64, sizeof be64);
        }

        ///
        /// Append int32_t using network endian
        ///
        void bb_append_int32(int32_t x)
        {
            int32_t be32 = w_sockets::host_2_network32(x);
            bb_append(&be32, sizeof be32);
        }

        void bb_append_int16(int16_t x)
        {
            int16_t be16 = w_sockets::host_2_network16(x);
            bb_append(&be16, sizeof be16);
        }

        void bb_append_int8(int8_t x)
        {
            bb_append(&x, sizeof x);
        }


        /// Read int64_t from network endian
        ///
        /// Require: buf->readableBytes() >= sizeof(int32_t)
        int64_t bb_read_int64()
        {
            int64_t result = bb_peek_int64();
            bb_retrieve_int64();
            return result;
        }

        ///
        /// Read int32_t from network endian
        ///
        /// Require: buf->readableBytes() >= sizeof(int32_t)
        int32_t bb_read_int32()
        {
            int32_t result = bb_peek_int32();
            bb_retrieve_int32();
            return result;
        }

        int16_t bb_read_int16()
        {
            int16_t result = bb_peek_int16();
            bb_retrieve_int16();
            return result;
        }

        int8_t bb_read_int8()
        {
            int8_t result = bb_peek_int8();
            bb_retrieve_int8();
            return result;
        }

        int64_t bb_peek_int64() const
        {
            if (bb_bytes_readable() < sizeof(int64_t))
                return -1;

            int64_t be64 = 0;
            ::memcpy(&be64, bb_peek(), sizeof be64);
            return w_sockets::network_2_host64(be64);
        }

        ///
        /// Peek int32_t from network endian
        ///
        /// Require: buf->readableBytes() >= sizeof(int32_t)
        int32_t bb_peek_int32() const
        {
            if (bb_bytes_readable() < sizeof(int32_t))
                return -1;

            int32_t be32 = 0;
            ::memcpy(&be32, bb_peek(), sizeof be32);
            return w_sockets::network_2_host32(be32);
        }

        int16_t bb_peek_int16() const
        {
            if (bb_bytes_readable() < sizeof(int16_t))
                return -1;

            int16_t be16 = 0;
            ::memcpy(&be16, bb_peek(), sizeof be16);
            return w_sockets::network_2_host16(be16);
        }

        int8_t bb_peek_int8() const
        {
            if (bb_bytes_readable() < sizeof(int8_t))
                return -1;

            int8_t x = *bb_peek();
            return x;
        }

        void bb_prepend_int64(int64_t x)
        {
            int64_t be64 = w_sockets::host_2_network64(x);
            bb_prepend(&be64, sizeof be64);
        }

        ///
        /// Prepend int32_t using network endian
        ///
        void bb_prepend_int32(int32_t x)
        {
            int32_t be32 = w_sockets::host_2_network32(x);
            bb_prepend(&be32, sizeof be32);
        }

        void bb_prepend_int16(int16_t x)
        {
            int16_t be16 = w_sockets::host_2_network16(x);
            bb_prepend(&be16, sizeof be16);
        }

        void bb_prepend_int8(int8_t x)
        {
            bb_prepend(&x, sizeof x);
        }

        bool bb_prepend(const void* /*restrict*/ data, size_t len)
        {
            if (len > bb_bytes_prependable())
                return false;

            read_idx_ -= len;
            const char* d = static_cast<const char*>(data);
            std::copy(d, d + len, _begin() + read_idx_);
            return true;
        }

        void bb_shrink(size_t reserve)
        {
            // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
            ByteBuffer other;
            other.bb_bytes_check_writable(bb_bytes_readable() + reserve);
            other.bb_append(bb_2_string_piece());
            bb_swap(other);
        }

        size_t bb_internal_capacity() const
        {
            return buffer_.capacity();
        }

        /// Read data directly into buffer.
        ///
        /// It may implement with readv(2)
        /// @return result of read(2), @c errno is saved
        int32_t bb_read_fd(int fd, int* saved_errno);        

    private:
        char* _begin()
        {
            return &*buffer_.begin();
        }
        
        const char* _begin() const
        {
            return &*buffer_.begin();
        }

        void _make_zoom(size_t len)
        {
            //kCheapPrepend为保留的空间
            if (bb_bytes_writeable() + bb_bytes_prependable() < len + cheap_prepend)
            {
                // FIXME: move readable data
                buffer_.resize(write_idx_+ len);
            }
            else
            {
                // move readable data to the front, make space inside buffer
                //assert(kCheapPrepend < readerIndex_);
                if (cheap_prepend >= read_idx_)
                    return;

                size_t readable = bb_bytes_readable();
                std::copy(_begin() + read_idx_,
                    _begin() + write_idx_,
                    _begin() + cheap_prepend);
                    read_idx_ = cheap_prepend;
                    write_idx_ = read_idx_ + readable;
            }
        }

    private:
        std::vector<char> buffer_;
        size_t read_idx_;
        size_t write_idx_;

        static const char CLRF_[];
    };
    
}
