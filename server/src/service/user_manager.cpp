/**
 *  管理所有的用户信息，初始信息从数据库中加载, UserManager.h
 *  zhangyl 2017.03.15
 **/
#include "user_manager.h"
#include <memory>
#include <sstream>
#include <stdio.h>
#include "whisp_log.h"
// #include "../mysqlapi/DatabaseMysql.h"
// #include "../base/AsyncLog.h"
// #include "../jsoncpp1.9.0/json.h"

UserManager::UserManager()
{
    
}

UserManager::~UserManager()
{

}

bool UserManager::init(const char* dbServer, const char* dbUserName, const char* dbPassword, const char* dbName)
{
    db_server_ = dbServer;
    db_user_name_ = dbUserName;
    if (dbPassword != NULL)
        db_password_ = dbPassword;
    db_name_ = dbName;

    //从数据库中加载所有用户信息
    if (!loadUsersFromDb())
        return false;

    //TODO: 当用户比较多，这个循环比较慢，优化之
    for (auto& iter : all_cached_users_)
    {
        if (!loadRelationshipFromDb(iter.userid, iter.friends))
        {
            WHISP_LOG_ERROR("Load relationship from db error, userid: %d", iter.userid);
            continue;
        }
    }

    return true;
}

bool UserManager::loadUsersFromDb()
{  
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_FALTAL("UserManager::LoadUsersFromDb failed, please check params: dbserver: %s, dbusername: %s, , dbpassword: %s, dbname: %s",
                   db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    //TODO: 到底是空数据集还是出错，需要修改下返回类型
    QueryResult* pResult = pConn->query("SELECT f_user_id, f_username, f_nickname, f_password,  f_facetype, f_customface, f_gender, f_birthday, f_signature, f_address, f_phonenumber, f_mail, f_teaminfo FROM t_user ORDER BY  f_user_id DESC");
    if (NULL == pResult)
    {
        WHISP_LOG_INFO("UserManager::_Query error, dbname: %s", db_name_.c_str());
        return false;
    }
    
    string teaminfo;
    while (true)
    {
        Field* pRow = pResult->fetch();
        if (pRow == NULL)
            break;
        
        User u;
        u.userid = pRow[0].getInt32();
        u.username = pRow[1].getString();
        u.nickname = pRow[2].getString();
        u.password = pRow[3].getString();
        u.facetype = pRow[4].getInt32();
        u.customface = pRow[5].getString();
        u.gender = pRow[6].getInt32();
        u.birthday = pRow[7].getInt32();
        u.signature = pRow[8].getString();
        u.address = pRow[9].getString();
        u.phonenumber = pRow[10].getString();
        u.mail = pRow[11].getString();
        u.teaminfo = pRow[12].getString();
        all_cached_users_.push_back(u);

        WHISP_LOG_INFO("userid: %d, username: %s, password: %s, nickname: %s, signature: %s", u.userid, u.username.c_str(), u.password.c_str(), u.nickname.c_str(), u.signature.c_str());
        
        //计算当前最大userid
        if (u.userid < GROUPID_BOUBDARY && u.userid > base_user_id_)
            base_user_id_ = u.userid;

        //计算当前最大群组id
        if (u.userid > GROUPID_BOUBDARY && u.userid > base_group_id_)
            base_group_id_ = u.userid;

        if (!pResult->nextRow())
        {
            break;
        }
    }

    WHISP_LOG_INFO("current base userid: %d, current base group id: %d", base_user_id_.load(), base_group_id_.load());

    pResult->endQuery();

    delete pResult;

    return true;
}

bool UserManager::addUser(User& u)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::AddUser failed, please check params: dbserver: %s, , dbusername: %s, dbpassword: %s, dbname: %s", db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    ++ base_user_id_;
    char sql[256] = { 0 };
    snprintf(sql, 256, "INSERT INTO t_user(f_user_id, f_username, f_nickname, f_password, f_register_time) VALUES(%d, '%s', '%s', '%s', NOW())", base_user_id_.load(), u.username.c_str(), u.nickname.c_str(), u.password.c_str());
    if (!pConn->execute(sql))
    {
        WHISP_LOG_WARN("insert user error, sql: %s", sql);
        return false;
    }
    //设置一些字段的默认值
    u.userid = base_user_id_;
    u.facetype = 0;
    u.birthday = 19900101;
    u.gender = 0;
    u.ownerid = 0;

    {
        std::lock_guard<std::mutex> guard(mutex_);
        all_cached_users_.push_back(u);
    }

    return true;
}

//数据库里面互为好友的两个人id，小者在先，大者在后
bool UserManager::makeFriendRelationshipInDB(int32_t smallUserid, int32_t greaterUserid)
{
    if (smallUserid == greaterUserid)
        return false;

    if (smallUserid > greaterUserid)
    {
        int32_t tmp = greaterUserid;
        greaterUserid = smallUserid;
        smallUserid = tmp;
    }

    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::LoadUsersFromDb failed, please check params: dbserver: %s, dbusername: %s, dbpassword: %s, , dbname: %s", 
             db_server_.c_str(),  db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    char sql[512] = { 0 };
    snprintf(sql, 512, "INSERT INTO t_user_relationship(f_user_id1, f_user_id2, f_user1_teamname, f_user2_teamname) VALUES(%d, %d, '%s', '%s')", smallUserid, greaterUserid, DEFAULT_TEAMNAME, DEFAULT_TEAMNAME);
    if (!pConn->execute(sql))
    {
        WHISP_LOG_ERROR("make relationship error, sql: %s, smallUserid: %s, greaterUserid: %s", sql, smallUserid , greaterUserid);
        return false;
    }
    
    //if (!AddFriendToUser(smallUserid, greaterUserid))
    //{
    //    WHISP_LOG_ERROR << "make relationship error, smallUserid=" << smallUserid << ", greaterUserid=" << greaterUserid;
    //    return false;
    //}

    return true;
}

bool UserManager::releaseFriendRelationshipInDBAndMemory(int32_t smallUserid, int32_t greaterUserid)
{
    if(smallUserid == greaterUserid)
        return false;

    if (smallUserid > greaterUserid)
    {
        int32_t tmp = greaterUserid;
        greaterUserid = smallUserid;
        smallUserid = tmp;
    }

    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::LoadUsersFromDb failed, please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s",
            db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    char sql[256] = { 0 };
    snprintf(sql, 256, "DELETE FROM t_user_relationship WHERE f_user_id1 = %d AND f_user_id2 = %d", smallUserid, greaterUserid);
    if (!pConn->execute(sql))
    {
        WHISP_LOG_ERROR("release relationship error, sql: %s, smallUserid: %d, , greaterUserid: %d", sql, smallUserid, greaterUserid);
        return false;
    }

    if (!deleteFriendToUser(smallUserid, greaterUserid))
    {
        WHISP_LOG_ERROR("delete relationship error, smallUserid: %d, , greaterUserid: %d", smallUserid, greaterUserid);
        return false;
    }

    return true;
}

bool UserManager::updateUserRelationshipInMemory(int32_t userid, int32_t target, FRIEND_OPERATION operation)
{
    if (operation == FRIEND_OPERATION_ADD)
    {
        bool found1 = false;
        bool found2 = false;
        {
            std::lock_guard<std::mutex> guard(mutex_);
            for (auto& iter : all_cached_users_)
            {
                if (iter.userid == userid)
                {
                    FriendInfo fi = { target, "", DEFAULT_TEAMNAME };
                    iter.friends.emplace_back(fi);
                    found1 = true;
                    continue;;
                }

                if (iter.userid == target)
                {
                    FriendInfo fi = { userid, "", DEFAULT_TEAMNAME };
                    iter.friends.emplace_back(fi);
                    found2 = true;
                    continue;
                }
            }
        }

        if (found1 && found2)
            return true;

        return false;
    }
    else if (operation == FRIEND_OPERATION_DELETE)
    {
        bool found1 = false;
        bool found2 = false;
        {
            std::lock_guard<std::mutex> guard(mutex_);
            for (auto& iter : all_cached_users_)
            {
                if (iter.userid == userid)
                {
                    for (auto iter2 = iter.friends.begin(); iter2 != iter.friends.end(); ++iter2)
                    {
                        if (iter2->friendid == target)
                        {
                            iter.friends.erase(iter2);
                            found1 = true;
                            break;
                        }
                    }

                    if (found1)
                        continue;
                }

                if (iter.userid == target)
                {
                    for (auto iter3 = iter.friends.begin(); iter3 != iter.friends.end(); ++iter3)
                    {
                        if (iter3->friendid == userid)
                        {
                            iter.friends.erase(iter3);
                            found2 = true;
                            break;
                        }
                    }

                    if (found2)
                        continue;
                }
            }
        }

        if (found1 && found2)
            return true;

        return false;
    }

    return false;
}

bool UserManager::addFriendToUser(int32_t userid, int32_t friendid)
{
    bool bFound1 = false;
    bool bFound2 = false;
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            FriendInfo fi = { friendid, "", DEFAULT_TEAMNAME };
            iter.friends.emplace_back(fi);
            bFound1 = true;
        }

        if (iter.userid == friendid)
        {
            FriendInfo fi = { userid, "", DEFAULT_TEAMNAME };
            iter.friends.emplace_back(fi);
            bFound2 = true;
        }

        if (bFound1 && bFound2)
            return true;
    }

    return false;
}

bool UserManager::deleteFriendToUser(int32_t userid, int32_t friendid)
{
    bool bFound1 = false;
    bool bFound2 = false;
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            for (auto iter2 = iter.friends.begin(); iter2 != iter.friends.end(); ++iter2)
            {
                if (iter2->friendid == friendid)
                {
                    iter.friends.erase(iter2);
                    bFound1 = true;
                    break;
                }
            }
            
            if (bFound1)
                continue;
        }

        if (iter.userid == friendid)
        {
            for (auto iter2 = iter.friends.begin(); iter2 != iter.friends.end(); ++iter2)
            {
                if (iter2->friendid == userid)
                {
                    iter.friends.erase(iter2);
                    bFound2 = true;
                    break;
                }
            }

            if (bFound2)
                continue;
        }   
    }

    if (bFound1 && bFound2)
        return true;

    return false;
}

bool UserManager::isFriend(int32_t userid, int32_t friendid)
{
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            for (auto iter2 = iter.friends.begin(); iter2 != iter.friends.end(); ++iter2)
            {
                if (iter2->friendid == friendid)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool UserManager::updateUserInfoInDb(int32_t userid, const User& newuserinfo)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::Initialize db failed,  please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s",
            db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    std::ostringstream osSql;
    osSql << "UPDATE t_user SET f_nickname='"        
          << newuserinfo.nickname << "', f_facetype=" 
          << newuserinfo.facetype << ", f_customface='" 
          << newuserinfo.customface << "', f_gender=" 
          << newuserinfo.gender << ", f_birthday=" 
          << newuserinfo.birthday << ", f_signature='" 
          << newuserinfo.signature << "', f_address='" 
          << newuserinfo.address << "', f_phonenumber='" 
          << newuserinfo.phonenumber << "', f_mail='" 
          << newuserinfo.mail << "' WHERE f_user_id=" 
          << userid;
    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("UpdateUserInfo error, sql: %s", osSql.str().c_str());
        return false;
    }

    WHISP_LOG_INFO("update userinfo successfully, userid: %d, sql: %s", userid, osSql.str().c_str());

    std::lock_guard<std::mutex> guard(mutex_);
    for (auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            iter.nickname = newuserinfo.nickname;
            iter.facetype = newuserinfo.facetype;
            iter.customface = newuserinfo.customface;
            iter.gender = newuserinfo.gender;
            iter.birthday = newuserinfo.birthday;
            iter.signature = newuserinfo.signature;
            iter.address = newuserinfo.address;
            iter.phonenumber = newuserinfo.phonenumber;
            iter.mail = newuserinfo.mail;
            return true;
        }
    }

    WHISP_LOG_ERROR("Failed to update userinfo to db, find exsit user in memory error, all_cached_users_.size(): %d, userid: %d, sql: %s", all_cached_users_.size(), userid, osSql.str().c_str());

    return false;
}

bool UserManager::modifyUserPassword(int32_t userid, const std::string& newpassword)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::Initialize db failed,  please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s",
            db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    std::ostringstream osSql;
    osSql << "UPDATE t_user SET f_password='"
        << newpassword << "' WHERE f_user_id="
        << userid;
    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("UpdateUserInfo error, sql: %s", osSql.str().c_str());
        return false;
    }

    WHISP_LOG_INFO("update user password successfully, userid: %d, sql: %s" , userid, osSql.str().c_str());

    std::lock_guard<std::mutex> guard(mutex_);
    for (auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            iter.password = newpassword;         
            return true;
        }
    }

    WHISP_LOG_ERROR("Failed to update user password to db, find no exsit user in memory error, all_cached_users_.size(): %d, userid: %d, sql: %s", all_cached_users_.size(), userid, osSql.str().c_str());

    return false;
}

bool UserManager::updateUserTeamInfoInDbAndMemory(int32_t userid, const std::string& newteaminfo)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::Initialize db failed, please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s",
            db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    std::ostringstream osSql;
    osSql << "UPDATE t_user SET f_teaminfo='"
        << newteaminfo << "' WHERE f_user_id="
        << userid;
    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("Update Team Info error, sql: %s", osSql.str().c_str());
        return false;
    }

    WHISP_LOG_INFO("update user teaminfo successfully, userid: %d, sql: %s", userid, osSql.str().c_str());

    //TODO: 重复的代码，需要去掉
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            iter.teaminfo = newteaminfo;
            return true;
        }
    }

    WHISP_LOG_ERROR("Failed to update user teaminfo to db, find no exsit user in memory error, all_cached_users_.size(): %d, userid: %d, sql: %s", all_cached_users_.size(), userid, osSql.str().c_str());

    return false;
}

bool UserManager::deleteTeam(int32_t userid, const std::string& deletedteamname)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::Initialize db failed, please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s",
            db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    std::ostringstream osSql;
    osSql << "UPDATE t_user_relationship SET f_user1_teamname='"
          << DEFAULT_TEAMNAME << "' WHERE f_user_id1=" << userid 
          << " AND f_user1_teamname='" << deletedteamname << "'";
    

    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("DeleteTeam error, sql: %s, userid: %d, deletedteamname: %s", osSql.str().c_str(), userid, deletedteamname.c_str());
        return false;
    }

    osSql.str("");

    osSql << "UPDATE t_user_relationship SET f_user2_teamname='"
        << DEFAULT_TEAMNAME << "' WHERE f_user_id2=" << userid
        << " AND f_user2_teamname='" << deletedteamname << "'";

    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("DeleteTeam error, sql: %s, userid: %d, deletedteamname: %s", osSql.str().c_str(), userid, deletedteamname.c_str());
        return false;
    }

    {
        std::lock_guard<std::mutex> guard(mutex_);
        for (auto& iter : all_cached_users_)
        {
            if (iter.userid == userid)
            {
                for (auto& iter2 : iter.friends)
                {
                    if (iter2.teamname == deletedteamname)
                        iter2.teamname = DEFAULT_TEAMNAME;
                }

                return true;
            }
        }
    }
    
    return false; 
}

bool UserManager::modifyTeamName(int32_t userid, const std::string& newteamname, const std::string& oldteamname)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::Initialize db failed, please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s",
            db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    std::ostringstream osSql;
    osSql << "UPDATE t_user_relationship SET f_user1_teamname='"
        << newteamname << "' WHERE f_user_id1=" << userid
        << " AND f_user1_teamname='" << oldteamname << "'";


    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("ModifyTeamName error, sql: %s, userid: %d, newteamname: %s, oldteamname: %s", osSql.str().c_str(), userid, newteamname.c_str(), oldteamname.c_str());
        return false;
    }

    osSql.str("");

    osSql << "UPDATE t_user_relationship SET f_user2_teamname='"
        << newteamname << "' WHERE f_user_id2=" << userid
        << " AND f_user2_teamname='" << oldteamname << "'";

    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("ModifyTeamName error, sql: %s, userid: %d, newteamname: %s, oldteamname: %s", osSql.str().c_str(), userid, newteamname.c_str(), oldteamname.c_str());
        return false;
    }

     {
         std::lock_guard<std::mutex> guard(mutex_);
         for (auto& iter : all_cached_users_)
         {
             if (iter.userid == userid)
             {
                 for (auto& iter2 : iter.friends)
                 {
                     if (iter2.teamname == oldteamname)
                         iter2.teamname = newteamname;
                 }
                 return true;
             }
         }
     }

    return true;
}

bool UserManager::updateMarknameInDb(int32_t userid, int32_t friendid, const std::string& newmarkname)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::Initialize db failed, please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s",
             db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    std::ostringstream osSql;
    if (userid < friendid)
    {
        osSql << "UPDATE t_user_relationship SET f_user1_markname='"
            << newmarkname << "' WHERE f_user_id1="
            << userid << " AND f_user_id2=" << friendid;
    }
    else
    {
        osSql << "UPDATE t_user_relationship SET f_user2_markname='"
            << newmarkname << "' WHERE f_user_id2="
            << userid << " AND f_user_id1=" << friendid;
    }
    
    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("Update Markname error, sql: %s", osSql.str().c_str());
        return false;
    }

    WHISP_LOG_INFO("update markname successfully, userid: %d, friendid: %d, sql: %s", userid, friendid, osSql.str().c_str());

    //TODO: 重复的代码，需要去掉
    std::lock_guard<std::mutex> guard(mutex_);
    std::set<FriendInfo> friends;
    for (auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            for (auto& iter2 : iter.friends)
            {
                if (iter2.friendid == friendid)
                {
                    iter2.markname = newmarkname;
                    return true;
                }
            }          
        }
    }

    WHISP_LOG_ERROR("Failed to update markname, find no exsit user in memory error, all_cached_users_.size(): %d, userid: %d, friendid: %d, sql: %s",
         all_cached_users_.size(), userid, friendid, osSql.str().c_str());

    return false;
}

bool UserManager::moveFriendToOtherTeam(int32_t userid, int32_t friendid, const std::string& newteamname)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::Initialize db failed, please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s, userid: %d, friendid: %d, newteamname: %s",
             db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str(), userid, friendid, newteamname.c_str());
        return false;
    }

    std::ostringstream osSql;
    if (userid < friendid)
    {
        osSql << "UPDATE t_user_relationship SET f_user1_teamname='"
              << newteamname << "' WHERE f_user_id1="
              << userid << " AND f_user_id2=" << friendid;
    }
    else
    {
        osSql << "UPDATE t_user_relationship SET f_user2_teamname='"
              << newteamname << "' WHERE f_user_id1="
              << friendid << " AND f_user_id2=" << userid;
    }

    if (!pConn->execute(osSql.str().c_str()))
    {
        WHISP_LOG_ERROR("MoveFriendToOtherTeam, sql: %s, userid: %d, friendid: %d, newteamname: %s", osSql.str().c_str(), userid, friendid, newteamname.c_str());
        return false;
    }

    WHISP_LOG_INFO("MoveFriendToOtherTeam db operation successfully, userid: %d, friendid: %d, sql: %s" , userid, friendid, osSql.str().c_str());

    //改变内存中用户的分组信息
    User* u = NULL;
    if (!getUserInfoByUserId(userid, u) || u == NULL)
    {
        WHISP_LOG_ERROR("MoveFriendToOtherTeam memory operation error, userid: %d, friendid: %d" , userid, friendid);
        return false;
    }

    for (auto& f : u->friends)
    {
        if (f.friendid == friendid)
        {
            f.teamname = newteamname;
            return true;
        }
    }

    return false;
}

bool UserManager::addGroup(const char* groupname, int32_t ownerid, int32_t& groupid)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::AddGroup failed, please check params: dbserver: %s, dbusername: %s, dbname: %s, dbpassword: %s, groupname: %s, ownerid: %d",
            db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str(), groupname, ownerid);
        return false;
    }

    ++base_group_id_;
    char sql[256] = { 0 };
    snprintf(sql, 256, "INSERT INTO t_user(f_user_id, f_username, f_nickname, f_password, f_owner_id, f_register_time) VALUES(%d, '%d', '%s', '', %d,  NOW())", base_group_id_.load(), base_group_id_.load(), groupname, ownerid);
    if (!pConn->execute(sql))
    {
        WHISP_LOG_ERROR("insert group error, sql: %s", sql);
        return false;
    }
    
    groupid = base_group_id_;

    User u;
    u.userid = groupid;
    char szUserName[12] = { 0 };
    snprintf(szUserName, 12, "%d", groupid);
    u.username = szUserName;
    u.nickname = groupname;
    u.ownerid = ownerid;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        all_cached_users_.push_back(u);
    }

    return true;
}

bool UserManager::saveChatMsgToDb(int32_t senderid, int32_t targetid, const std::string& chatmsg)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_ERROR("UserManager::SaveChatMsgToDb failed, please check params: dbserver: %s, dbusername: %s, dbpassword: %s, dbname: %s", db_server_.c_str(), db_user_name_.c_str(), db_password_.c_str(), db_name_.c_str());
        return false;
    }

    ostringstream sql;
    sql << "INSERT INTO t_chatmsg(f_senderid, f_targetid, f_msgcontent) VALUES(" << senderid << ", " << targetid << ", '" << chatmsg << "')";
    if (!pConn->execute(sql.str().c_str()))
    {
        WHISP_LOG_ERROR("UserManager::SaveChatMsgToDb, sql: %s, senderid: %d, targetid: %d, chatmsg: %s", sql.str().c_str(), senderid, targetid, chatmsg.c_str());
        return false;
    }

    return true;
}

bool UserManager::getUserInfoByUsername(const std::string& username, User& u)
{
    std::lock_guard<std::mutex> guard(mutex_);
    for (const auto& iter : all_cached_users_)
    {
        if (iter.username == username)
        {
            u = iter;
            return true;
        }
    }

    return false;
}

bool UserManager::getUserInfoByUserId(int32_t userid, User& u)
{
    std::lock_guard<std::mutex> guard(mutex_);
    for (const auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            u = iter;
            return true;
        }
    }

    return false;
}

bool UserManager::getUserInfoByUserId(int32_t userid, User*& u)
{
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            u = &iter;
            return true;
        }
    }

    return false;
}

bool UserManager::getFriendInfoByUserId(int32_t userid, std::list<User>& friends)
{
    std::list<FriendInfo> friendInfo;
    std::lock_guard<std::mutex> guard(mutex_);
    //先找到friends的id列表
    for (const auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            friendInfo = iter.friends;
            break;
        }
    }

    //再通过每个friendid找到对应的User集合
    //TODO: 这种算法效率太低
    for (const auto& iter : friendInfo)
    {
        User u;
        for (const auto& iter2 : all_cached_users_)
        {
            if (iter2.userid == iter.friendid)
            {
                u = iter2;
                friends.push_back(u);
                break;
            }
        }
    }

    return true;
}

bool UserManager::getFriendMarknameByUserId(int32_t userid1, int32_t friendid, std::string& markname)
{
    std::list<FriendInfo> friendInfo;
    std::lock_guard<std::mutex> guard(mutex_);
    //先找到friends的id列表
    for (const auto& iter : all_cached_users_)
    {
        if (iter.userid == userid1)
        {
            friendInfo = iter.friends;
            for (const auto& iter2 : friendInfo)
            {
                if (iter2.friendid == friendid)
                {
                    markname = iter2.markname;
                    return true;
                }
            }         
        }
    }

    return false;
}

bool UserManager::getTeamInfoByUserId(int32_t userid, std::string& teaminfo)
{
    std::set<int32_t> friendsId;
    std::lock_guard<std::mutex> guard(mutex_);
    for (const auto& iter : all_cached_users_)
    {
        if (iter.userid == userid)
        {
            teaminfo = iter.teaminfo;
            return true;
        }
    }
    
    return false;
}

bool UserManager::loadRelationshipFromDb(int32_t userid, std::list<FriendInfo>& r)
{
    std::unique_ptr<CDatabaseMysql> pConn;
    pConn.reset(new CDatabaseMysql());
    if (!pConn->initialize(db_server_, db_user_name_, db_password_, db_name_))
    {
        WHISP_LOG_FALTAL("UserManager::LoadRelationhipFromDb failed, please check params");
        return false;
    }

    char sql[256] = { 0 };
    snprintf(sql, 256, "SELECT f_user_id1, f_user_id2, f_user1_markname, f_user2_markname, f_user1_teamname, f_user2_teamname FROM t_user_relationship WHERE f_user_id1 = %d OR f_user_id2 = %d ", userid, userid);
    QueryResult* pResult = pConn->query(sql);
    if (NULL == pResult)
    {
        WHISP_LOG_INFO("UserManager::Query error, db: %s", db_name_.c_str());
        return false;
    }

    while (true)
    {
        Field* pRow = pResult->fetch();
        if (pRow == NULL)
            break;

        int32_t friendid1 = pRow[0].getInt32();
        int32_t friendid2 = pRow[1].getInt32();
        string markname1 = pRow[2].getCppString();
        string markname2 = pRow[3].getCppString();  
        string teamname1 = pRow[4].getCppString();
        string teamname2 = pRow[5].getCppString();
        if (teamname1.empty())
            teamname1 = DEFAULT_TEAMNAME;
        if (teamname2.empty())
            teamname2 = DEFAULT_TEAMNAME;
        FriendInfo fi;
        if (friendid1 == userid)
        {
            fi.friendid = friendid2;
            fi.markname = markname1;
            fi.teamname = teamname1;
            r.emplace_back(fi);
            WHISP_LOG_INFO("userid=%d, friendid=%d", userid, friendid2);
        }
        else
        {
            fi.friendid = friendid1;
            fi.markname = markname2;
            fi.teamname = teamname2;
            r.emplace_back(fi);
            WHISP_LOG_INFO("userid=%d, friendid=%d", userid, friendid2);            
        }
       
        if (!pResult->nextRow())
        {
            break;
        }
    }

    pResult->endQuery();

    delete pResult;
    
    return true;
}