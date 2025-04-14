#include "http_session.h"
#include "common/stringutil.h"
#include "util/urlencode_util.h"
#include "whisp_log.h"
// #include <stream.h>
#include <sstream>

#define MAX_URL_LENGTH 2048

HttpSession::HttpSession(std::shared_ptr<TcpConnection>& conn) : tmp_conn_(conn)
{

}

void HttpSession::on_read(const std::shared_ptr<TcpConnection>& conn, ByteBuffer* pBuffer, Timestamp receivTime)
{
    //WHISP_LOG_INFO << "Recv a http request from " << conn->peerAddress().toIpPort();
    
    std::string inbuf;
    //先把所有数据都取出来
    inbuf.append(pBuffer->bb_peek(), pBuffer->bb_bytes_readable());
    //因为一个http包头的数据至少\r\n\r\n，所以大于4个字符
    //小于等于4个字符，说明数据未收完，退出，等待网络底层接着收取
    if (inbuf.length() <= 4)
        return;

    //我们收到的GET请求数据包一般格式如下：
    /*
    GET /register.do?p={%22username%22:%20%2213917043329%22,%20%22nickname%22:%20%22balloon%22,%20%22password%22:%20%22123%22} HTTP/1.1\r\n
    Host: 120.55.94.78:12345\r\n
    Connection: keep-alive\r\n
    Upgrade-Insecure-Requests: 1\r\n
    User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/65.0.3325.146 Safari/537.36\r\n
    Accept-Encoding: gzip, deflate\r\n
    Accept-Language: zh-CN, zh; q=0.9, en; q=0.8\r\n
    \r\n
     */
    //检查是否以\r\n\r\n结束，如果不是说明包头不完整，退出
    std::string end = inbuf.substr(inbuf.length() - 4);
    if (end != "\r\n\r\n")
        return;
    //超过2048个字符，且不含\r\n\r\n，我们认为是非法请求
    else if (inbuf.length() >= MAX_URL_LENGTH)
    {
        conn->force_close();
        return;
    }

    //以\r\n分割每一行
    std::vector<std::string> lines;
    StringUtil::split(inbuf, lines, "\r\n");
    if (lines.size() < 1 || lines[0].empty())
    {
        conn->force_close();
        return;
    }

    std::vector<std::string> chunk;
    StringUtil::split(lines[0], chunk, " ");
    //chunk中至少有三个字符串：GET+url+HTTP版本号
    if (chunk.size() < 3)
    {
        conn->force_close();
        return;
    }

    WHISP_LOG_INFO("url: %s  from %s", chunk[1].c_str(), conn->peer_address().inet_2_ipport().c_str());
    //inbuf = /register.do?p={%22username%22:%20%2213917043329%22,%20%22nickname%22:%20%22balloon%22,%20%22password%22:%20%22123%22}
    std::vector<std::string> part;
    //通过?分割成前后两端，前面是url，后面是参数
    StringUtil::split(chunk[1], part, "?");
    //chunk中至少有三个字符串：GET+url+HTTP版本号
    if (part.size() < 2)
    {
        conn->force_close();
        return;
    }

    std::string url = part[0];
    std::string param = part[1].substr(2);
        
    if (!_process(conn, url, param))
    {
        WHISP_LOG_ERROR("handle http request error, from: %s, request: %s", conn->peer_address().inet_2_ipport().c_str(), pBuffer->bb_retrieve_all_as_string().c_str());
    }

    //短连接，处理完关闭连接
    conn->force_close();
}

void HttpSession::send(const char* data, size_t length)
{
    if (!tmp_conn_.expired())
    {
        std::shared_ptr<TcpConnection> conn = tmp_conn_.lock();
        conn->send(data, length);
    }
}

bool HttpSession::_process(const std::shared_ptr<TcpConnection>& conn, const std::string& url, const std::string& param)
{
    if (url.empty())
        return false;

    if (url == "/register.do")
    {
        _on_register_response(param, conn);
    }
    else if (url == "/login.do")
    {
        _on_login_response(param, conn);
    }
    else if (url == "/getfriendlist.do")
    {

    }
    else if (url == "/getgroupmembers.do")
    {

    }
    else
        return false;

    
    return true;
}

//组装http协议应答包
/* 
    HTTP/1.1 200 OK\r\n
    Content-Type: text/html\r\n   
    Date: Wed, 16 May 2018 10:06:10 GMT\r\n
    Content-Length: 4864\r\n
    \r\n\
    {"code": 0, "msg": ok}
*/
void HttpSession::_makeup_response(const std::string& input, std::string& output)
{ 
    std::ostringstream os;
    os << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length:"
        << input.length() << "\r\n\r\n"
        << input;

    output = os.str();
}

void HttpSession::_on_register_response(const std::string& data, const std::shared_ptr<TcpConnection>& conn)
{
    std::string retData;
    std::string decodeData;
    URLEncodeUtil::decode(data, decodeData);
    BussinessLogic::register_user(decodeData, conn, false, retData);
    if (!retData.empty())
    {
        std::string response;
        URLEncodeUtil::encode(retData, response);
        _makeup_response(retData, response);
        conn->send(response);

        WHISP_LOG_INFO("Response to client: cmd=msg_type_register, data: %s, client: %s", retData.c_str(), conn->peer_address().inet_2_ipport().c_str());
    }
}

void HttpSession::_on_login_response(const std::string& data, const std::shared_ptr<TcpConnection>& conn)
{

}