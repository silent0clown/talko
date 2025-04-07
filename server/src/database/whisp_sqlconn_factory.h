#ifndef WHISP_SQLCONN_FACTORY_H
#define WHISP_SQLCONN_FACTORY_H
#include "whisp_mysqlconn_pool.h"
#include <iostream>
#include <memory>

// 抽象数据库连接池工厂类
class WhispAbstractDbConnFactory {
public:
    virtual std::shared_ptr<MysqlConnPool> create_mysqlconn_pool() = 0;
    virtual void connect() = 0;
    virtual ~WhispAbstractDbConnFactory() {}

protected:
    WhispAbstractDbConnFactory() = default;
};

// 具体工厂类：实现MySQL连接池的创建
class WhispConcreteDbConnFactory : public WhispAbstractDbConnFactory {
public:
    WhispConcreteDbConnFactory(const std::string& host, const int port, 
                               const std::string& user, const std::string& password, 
                               const std::string& database, size_t pool_size)
        : host_(host), port_(port), user_(user), password_(password), 
          database_(database), pool_size_(pool_size) {}

    std::shared_ptr<MysqlConnPool> create_mysqlconn_pool() override {
        // 使用连接参数创建MysqlConnPool对象
        return std::make_shared<MysqlConnPool>(host_, port_, user_, password_, database_, pool_size_);
    }

    void connect() override {
        // std::cout << "Connecting to the database..." << std::endl;
        // 你可以在这里加入实际的连接逻辑
    }

private:
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;
    size_t pool_size_;
};

#endif // WHISP_SQLCONN_FACTORY_H