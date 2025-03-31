#ifndef WHISP_DB_H
#define WHISP_DB_H

#include "db_user_dao.h"
#include "db_mysqlconn_pool.h"


class WhispDB {
public:
    // 单例模式（可选），确保全局只创建一个数据库实例
    /*
    static WhispDB get_instance() {    // WhispDB&：这是函数的返回类型，表示返回一个WhispDB类的引用（即 WhispDB&）。通过返回引用，可以避免复制 WhispDB 对象，同时也能保证所有的操作都作用在同一个实例上。
        static WhispDB instance;    // C++11及以上版本是线程安全的
        return instance;
    }
    */
    static WhispDB* get_instance() {
        if (instance_ == nullptr) {
            std::lock_guard<std::mutex> guard(mutex_);
            if (instance_ == nullptr) {
                instance_ = new WhispDB();
                
            }
        }
        return instance_;
    }
    
    // 禁止拷贝 & 赋值
    WhispDB(const WhispDB&) = delete;
    WhispDB& operator=(const WhispDB&) = delete;
    
    // 统一初始化数据库
    bool init(const std::string& db_host, const std::string& db_user, const std::string& db_password, 
            const std::string& db_name, int pool_size = 10) {
        return conn_pool_.init(db_host, db_user, db_password, db_name, pool_size);
    }
    
    // 获取用户 DAO，外部通过 WhispDB 访问 DAO 层
    WhispUserDAO& get_user_dao() {
        return user_dao_;
    }
        
private:
    static WhispDB* instance_;
    static std::mutex mutex_;

    WhispDB() : user_dao_(conn_pool_) {}  // 依赖注入，WhispUserDAO 通过 WhispMysqlConnPool 访问数据库
    
    MysqlConnPool conn_pool_; // 连接池
    UserDAO user_dao_;        // 用户 DAO

};

// 在C++中，静态成员变量不能在类的头文件中直接进行初始化。你只能在类的定义中声明它们，而初始化则必须在源文件中进行
WhispDB* WhispDB::instance_ = nullptr;
std::mutex WhispDB::mutex_ ;


#endif // WHISP_DB_H
