#include <mysqlx/xdevapi.h>
#include <iostream>

int main() {
    try {
        // 1. 创建会话
        mysqlx::Session session("127.0.0.1", 6603, "root", "talko_root");

        // 2. 选择数据库（如果不存在则创建）
        mysqlx::Schema db = session.getSchema("talko_server", true);
        std::cout << "Connected to database: test_db" << std::endl;

        // 3. 创建表（如果不存在）
        mysqlx::Table table = db.getTable("user_info", true);
        if (!table.existsInDatabase()) {
            session.sql("CREATE TABLE user_info ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "username VARCHAR(255) NOT NULL, "
                        "password_hash VARCHAR(255) NOT NULL)"
                       ).execute();
            std::cout << "Table 'user_info' created." << std::endl;
        } else {
            std::cout << "Table 'user_info' already exists." << std::endl;
        }

        // 4. 插入数据
        table.insert("username", "password_hash")
             .values("user1", "hash1")
             .values("user2", "hash2")
             .execute();
        std::cout << "Data inserted into 'user_info'." << std::endl;

        // 5. 查询数据
        mysqlx::RowResult result = table.select("id", "username", "password_hash")
                                       .execute();
        std::cout << "Query results:" << std::endl;
        for (mysqlx::Row row : result) {
            std::cout << "ID: " << row[0] << ", "
                      << "Username: " << row[1] << ", "
                      << "Password Hash: " << row[2] << std::endl;
        }

        // 6. 关闭会话
        session.close();
        std::cout << "Session closed." << std::endl;
    } catch (const mysqlx::Error &err) {
        std::cerr << "MySQL Error: " << err.what() << std::endl;
    } catch (const std::exception &ex) {
        std::cerr << "Standard Exception: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown Exception!" << std::endl;
    }

    return 0;
}


