#ifndef __ZLIB_UTIL_H__
#define __ZLIB_UTIL_H__
#include <string>

class ZlibUtil
{
private:
    ZlibUtil() = delete;
    ~ZlibUtil() = delete;
    ZlibUtil(const ZlibUtil& rhs) = delete;

public:
    static bool compressBuf(const char* pSrcBuf, size_t nSrcBufLength, char* pDestBuf, size_t& nDestBufLength);
    static bool compressBuf(const std::string& strSrcBuf, std::string& strDestBuf);
    static bool uncompressBuf(const std::string& strSrcBuf, std::string& strDestBuf, size_t nDestBufLength);

    //gzipѹ��
    static bool inflate(const std::string& strSrc, std::string& dest);
    //gzip��ѹ
    static bool deflate(const std::string& strSrc, std::string& strDest);

};

#endif