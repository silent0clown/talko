#include "user_service.h"
#include <regex>
#include <chrono>
#include <iomanip>
#include <sstream>

UserService::UserService(DatabaseManager* dbManager) : dbManager_(dbManager) {}

std::string UserService::handleRegistration(const std::string& jsonRequest) {
    Json::Value request, response;
    Json::CharReaderBuilder readerBuilder;
    std::string errors;

    // 1. 解析JSON
    std::istringstream jsonStream(jsonRequest);
    if (!Json::parseFromStream(readerBuilder, jsonStream, &request, &errors)) {
        response["success"] = false;
        response["error"] = "Invalid JSON: " + errors;
        return Json::writeString(Json::StreamWriterBuilder(), response);
    }

    // 2. 校验必填字段
    if (!validateUserInfo(request)) {
        response["success"] = false;
        response["error"] = "Validation failed";
        return Json::writeString(Json::StreamWriterBuilder(), response);
    }

    std::string username = request["username"].asString();

    // 3. 检查用户是否存在
    if (dbManager_->userExists(username)) {
        response["success"] = false;
        response["error"] = "Username already exists";
        return Json::writeString(Json::StreamWriterBuilder(), response);
    }

    // 4. 构造UserInfo对象
    UserInfo newUser;
    newUser.username = username;
    newUser.password_hash = request["password_hash"].asString();
    newUser.nickname = request.get("nickname", "").asString();
    newUser.gender = request.get("gender", "other").asString();
    
    // 设置默认状态和时间戳
    newUser.status = "active";
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream timeStream;
    timeStream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    newUser.create_time = timeStream.str();
    newUser.update_time = newUser.create_time;

    // 5. 插入数据库
    if (!dbManager_->insertUser(newUser)) {
        response["success"] = false;
        response["error"] = "Database insertion failed";
    } else {
        response["success"] = true;
        response["user_id"] = newUser.id;
        response["message"] = "Registration successful";
    }

    return Json::writeString(Json::StreamWriterBuilder(), response);
}

bool UserService::validateUserInfo(const Json::Value& userData) {
    // 用户名校验
    static const std::regex usernameRegex("^[a-zA-Z0-9_]{4,20}$");
    if (!std::regex_match(userData["username"].asString(), usernameRegex)) {
        return false;
    }

    // 密码哈希校验（SHA-256格式）
    static const std::regex hashRegex("^[a-fA-F0-9]{64}$");
    if (!std::regex_match(userData["password_hash"].asString(), hashRegex)) {
        return false;
    }

    // 邮箱校验（可选字段）
    if (userData.isMember("email") && !userData["email"].asString().empty()) {
        static const std::regex emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        if (!std::regex_match(userData["email"].asString(), emailRegex)) {
            return false;
        }
    }

    // 其他字段校验...
    return true;
}