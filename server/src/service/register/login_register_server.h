#pragma once
#ifndef LOGIN_USER_SERVER
#define LOGIN_USER_SERVER

#include <string>
#include <json/json.h> // 推荐使用 nlohmann/json 或 RapidJSON
#include "db_user_info.h" 

class UserService {
public:
    explicit UserService(DatabaseManager* dbManager);
    std::string handleRegistration(const std::string& jsonRequest);

private:
    bool validateUserInfo(const Json::Value& userData);
    DatabaseManager* dbManager_;
};

#endif // LOGIN_USER_SERVER