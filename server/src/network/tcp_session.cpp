#include "tcp_session.h"
#include "protocol_stream.h"
#include "common/zlibutil.h"
#include "msg.h"
#include "whisp_log.h"

TcpSession::TcpSession(const std::weak_ptr<TcpConnection>& tmpconn): tmp_conn_(tmpconn)
{

}

TcpSession::~TcpSession()
{

}

void TcpSession::send(int32_t cmd, int32_t seq, const std::string& data)
{
    send(cmd, data.c_str(), data.length());
}

void TcpSession::send(int32_t cmd, int32_t seq, const char* data, int32_t data_len)
{
    std::string outbuf;
    w_network::BinaryStreamWriter write_stream(&outbuf);
    write_stream.WriteInt32(cmd);
    write_stream.WriteInt32(seq);
    write_stream.WriteCString(data, data_len);
    write_stream.Flush();

    send_pkg(outbuf.c_str(), outbuf.length());
}

void TcpSession::send(const std::string& outbuf)
{
    send_pkg(outbuf.c_str(), outbuf.length());
}


void TcpSession::send(const char* p, int32_t length)
{
    send_pkg(p, length);
}


void TcpSession::send_pkg(const char* p, int32_t length)
{   
    std::string srcbuf(p, length);
    std::string destbuf;
    if (!ZlibUtil::compressBuf(srcbuf, destbuf))
    {
        WHISP_LOG_ERROR("compress buf error");
        return;
    }
 
    std::string strPackageData;
    chat_msg_header header;
    header.compressflag = 1;
    header.compresssize = destbuf.length();
    header.originsize = length;
    // if (Singleton<ChatServer>::Instance().isLogPackageBinaryEnabled())
    // {
    //     LOGI("Send data, header length: %d, body length: %d", sizeof(header), destbuf.length());
    // }
    

    strPackageData.append((const char*)&header, sizeof(header));
    strPackageData.append(destbuf);


    if (tmp_conn_.expired())
    {

        WHISP_LOG_ERROR("Tcp connection is destroyed , but why TcpSession is still alive ?");
        return;
    }

    std::shared_ptr<TcpConnection> conn = tmp_conn_.lock();
    if (conn)
    {
        // if (Singleton<ChatServer>::Instance().isLogPackageBinaryEnabled())
        // {
        //     size_t length = strPackageData.length();
        //     LOGI("Send data, package length: %d", length);
        // }
        
        conn->send(strPackageData);
    }
}