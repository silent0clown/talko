#ifndef WHISP_MYSQLCONN_POOL_H
#define WHISP_MYSQLCONN_POOL_H

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <map>
#include <mysqlx/xdevapi.h>

// 抽象SQL连接类
class WhispSqlConn {
public:
    virtual ~WhispSqlConn() = 0;
    virtual bool connect(const std::string& host, const int port, const std::string& user, const std::string&passwrd, const std::string& database) = 0;
    virtual void disconnect() = 0;
    virtual bool execute(const std::string& sql) = 0;
protected:
    WhispSqlConn() = default;
private:
};


struct TableField
{
	TableField(){}
	TableField(std::string name,std::string type,std::string desc):
        name_(name),
        type_(type),
        desc_(desc)
	{
	}
	std::string name_;
	std::string type_;
	std::string desc_;
};

struct TableInfo
{
	TableInfo(){}
	TableInfo(std::string name) : name_(name)
	{
	}
	std::string name_;
	std::map<std::string, TableField> field_;
	std::string key_;
};



// Mysql连接类
class MysqlConn:public WhispSqlConn {
public:
    MysqlConn();
    ~MysqlConn() override;
    virtual bool connect(const std::string& host, const int port, const std::string& user, const std::string&passwrd, const std::string& database) override;
    virtual void disconnect() override;
    virtual bool execute(const std::string& sql) override;
protected:

private:
    mysqlx::Session* session;
};

// MySQL 连接池类
class MysqlConnPool {
public:
    MysqlConnPool(const std::string& host, const int port, const std::string& user, const std::string& password, const std::string& database, size_t poolSize);
    ~MysqlConnPool();
    std::shared_ptr<MysqlConn> get_conn(int timeout_ms=5000);
    // std::shared_ptr<MysqlConn> get_conn(int timeout_ms = 5000);
    void release_conn(std::shared_ptr<MysqlConn> conn);
private:
    bool _check_db_exist();
    bool _create_db();
    bool _check_table_exist(const TableInfo& table);
    bool _create_table(const TableInfo& table);
    bool _update_table(const TableInfo& table);

    std::queue<std::shared_ptr<MysqlConn>> conn_queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    size_t pool_size_;
    /* todo
    // bool init();  // 初始化连接池（创建连接）
       void destroy(); // 销毁连接池资源
       bool _is_conn_alive(std::shared_ptr<MysqlConn> conn);  // 心跳检查
    // std::atomic<size_t> active_count_    // 统计当前活跃链接数

    // 支持配置不同数据库实例或多租户

    // 支持连接池动态扩容（高级优化）

    功能	说明
✅ init() / destroy()	连接池初始化和销毁
✅ 连接健康检查	防止连接过期失效
✅ 超时控制	get_conn() 支持最大等待时间
✅ 自动归还连接	RAII方式避免忘记调用 release_conn()
✅ 连接失败自动重连	增强稳定性
✅ 活跃连接数追踪	实时监控
✅ 日志打印	异常、错误打日志
✅ 单例管理 or 多池管理	应对复杂业务场景

    */
};

#endif  // WHISP_MYSQLCONN_POOL_H