/**
 * 即时通讯的业务逻辑都统一放在这里，BusinessLogic.cpp
 */

 #include "bussiness_logic.h"
 #include "whisp_log.h"
 #include "json/json.h"
 #include "user_manager.h"
 #include "common/singleton.h"
 #include <string>
 
 void BussinessLogic::register_user(const std::string& data, const std::shared_ptr<TcpConnection>& conn, bool keepalive, std::string& retData)
 {
     //{ "user": "13917043329", "nickname" : "balloon", "password" : "123" }
 
     Json::CharReaderBuilder b;
     Json::CharReader* reader(b.newCharReader());
     Json::Value jsonRoot;
     JSONCPP_STRING errs;
     bool ok = reader->parse(data.c_str(), data.c_str() + data.length(), &jsonRoot, &errs);
     if (!ok || errs.size() != 0)
     {
         WHISP_LOG_WARN("invalid json: %s, client: %s", data.c_str(), conn->peer_address().inet_2_ipport().c_str());
         delete reader;
         return;
     }
     delete reader;
 
     if (!jsonRoot["username"].isString() || !jsonRoot["nickname"].isString() || !jsonRoot["password"].isString())
     {
         WHISP_LOG_WARN("invalid json: %s, client: %s", data.c_str(), conn->peer_address().inet_2_ipport().c_str());        
         return;
     }
 
     User u;
     u.username = jsonRoot["username"].asString();
     u.nickname = jsonRoot["nickname"].asString();
     u.password = jsonRoot["password"].asString();
 
     //std::string retData;
     User cachedUser;
     cachedUser.userid = 0;
     Singleton<UserManager>::Instance().getUserInfoByUsername(u.username, cachedUser);
     if (cachedUser.userid != 0) {
         retData = "{\"code\": 101, \"msg\": \"registered already\"}";
     }
     else
     {
         if (!Singleton<UserManager>::Instance().addUser(u))
             retData = "{\"code\": 100, \"msg\": \"register failed\"}";
         else
         {
             retData = "{\"code\": 0, \"msg\": \"ok\"}";
         }
     }
 
 
     //conn->Send(msg_type_register, m_seq, retData);
 
     //LOGI << "Response to client: cmd=msg_type_register" << ", userid=" << u.userid << ", data=" << retData;
 }