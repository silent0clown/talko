/*
 * dev API说明
*/

#include <mysqlx/xdevapi.h> // 引入 MySQL X DevAPI 头文件



/******************************* 数据库连接示例 *****************************/
{
    // 创建 MySQL 会话，连接到本地服务器的 33060 端口，使用 "user" 账户和 "password" 进行身份验证
    mysqlx::Session sess("localhost", 33060, "user", "password");

    // 获取 "test" 数据库（Schema），相当于 SQL 中的 `USE test;`
    mysqlx::Schema db = sess.getSchema("test");
    // 另一种获取 Schema 对象的方式
    // mysqlx::Schema db(sess, "test");

    // 获取 "test" 数据库中的集合 "my_collection"（适用于 JSON 文档存储）
    mysqlx::Collection myColl = db.getCollection("my_collection");
    // 另一种获取 Collection 对象的方式
    // mysqlx::Collection myColl(db, "my_collection");

    // 在 "my_collection" 集合中查找 name 字段符合 "L%" 模式的文档
    // 等价于 SQL: SELECT * FROM my_collection WHERE name LIKE 'L%' LIMIT 1;
    mysqlx::DocResult myDocs = myColl.find("name like :param") // 使用占位符 :param 进行查询
                                     .limit(1)                 // 限制返回 1 条数据
                                     .bind("param", "L%")      // 绑定 :param 为 "L%"
                                     .execute();               // 执行查询

    // 获取查询结果中的第一条文档并输出
    std::cout << myDocs.fetchOne();
}


/********************** 连接到单个 MySQL 服务器 *****************************/
// 获取用户名
string usr = prompt("Username:");
// 获取密码
string pwd = prompt("Password:");

// 连接到 MySQL 服务器（网络上的主机 "localhost" 的端口 33060），
// 使用用户输入的 `usr` 和 `pwd` 进行身份验证
mysqlx::Session mySession(
    mysqlx::SessionOption::HOST, "localhost",   // 指定主机
    mysqlx::SessionOption::PORT, 33060,        // 指定端口号
    mysqlx::SessionOption::USER, usr,          // 指定用户名
    mysqlx::SessionOption::PWD, pwd            // 指定密码
);

// 另一种定义会话设置的方式，使用 `SessionSettings` 配置对象
mysqlx::SessionSettings settings(
    mysqlx::SessionOption::HOST, "localhost",  // 指定主机
    mysqlx::SessionOption::PORT, 33060         // 指定端口号
);

// 通过 `set()` 方法动态设置用户名和密码
settings.set(mysqlx::SessionOption::USER, usr);
settings.set(mysqlx::SessionOption::PWD, pwd);

// 使用 `SessionSettings` 创建 MySQL 会话
mysqlx::Session mySession(settings);

// 获取 "test" 数据库（Schema），等价于 SQL: `USE test;`
mysqlx::Schema myDb = mySession.getSchema("test");



/********************** 使用连接池连接到单个 MySQL 服务器 *****************************/
// 引入 mysqlx 命名空间，避免每次使用时需要加 `mysqlx::`
using namespace mysqlx;

// 创建一个 MySQL 客户端对象 `cli`
// 连接到指定的 MySQL 服务器，格式为 `"user:password@host_name/db_name"`
// `ClientOption::POOL_MAX_SIZE, 7` 表示连接池最大允许 7 个会话
Client cli("user:password@host_name/db_name", ClientOption::POOL_MAX_SIZE, 7);

// 从客户端 `cli` 获取一个会话 `sess`，用于执行数据库操作
Session sess = cli.getSession();

// `sess` 现在可以像普通的 `Session` 对象一样使用
// 你可以执行查询、插入数据等操作

// 关闭 `Client`，这将关闭所有关联的 `Session`
cli.close();



/********************** 使用会话对象 *****************************/
#include <mysqlx/xdevapi.h>  // 引入 MySQL X DevAPI 头文件

// 连接到 MySQL 并创建 Session

// 使用连接 URI 连接到 MySQL 服务器
// "mysqlx://" 表示使用 X Protocol 连接
// "localhost:33060" 指定 MySQL 服务器地址和端口
// "test" 是目标数据库名
// "user=user&password=password" 指定用户名和密码
string url = "mysqlx://localhost:33060/test?user=user&password=password";

{
  // 创建一个 Session 对象 `mySession`，用于与 MySQL 服务器交互
  Session mySession(url);

  // 获取当前会话中所有可用的数据库（schemas）
  std::list<Schema> schemaList = mySession.getSchemas();

  // 输出可用的 schema 名称
  cout << "Available schemas in this session:" << endl;

  // 遍历 schemaList，打印每个 schema 的名称
  for (Schema schema : schemaList) {
    cout << schema.getName() << endl;
  }

  // 由于 `Session` 对象是局部变量，当作用域 `{}` 结束时，它会自动关闭连接
}



/********************** 在会话中使用 SQL *****************************/
#include <mysqlx/xdevapi.h>  // 引入 MySQL X DevAPI 头文件

// 连接到本地 MySQL 服务器
string url = "mysqlx://localhost:33060/test?user=user&password=password";
Session mySession(url);  // 创建 Session 对象，与 MySQL 服务器交互

// 切换到 `test` 数据库
mySession.sql("USE test").execute();

// 在 Session 上下文中，可以执行完整的 SQL 语句

// 创建一个存储过程 `my_add_one_procedure`，用于给传入的整数变量加 1
mySession.sql("CREATE PROCEDURE my_add_one_procedure "
              " (INOUT incr_param INT) "  // 传入和传出参数 `incr_param`
              "BEGIN "
              "  SET incr_param = incr_param + 1;"  // 变量 `incr_param` 自增 1
              "END;")
         .execute();

// 定义一个用户变量 `@my_var` 并赋值 10
mySession.sql("SET @my_var = ?;").bind(10).execute();

// 调用 `my_add_one_procedure` 存储过程，对 `@my_var` 进行 +1 操作
mySession.sql("CALL my_add_one_procedure(@my_var);").execute();

// 删除存储过程 `my_add_one_procedure`
mySession.sql("DROP PROCEDURE my_add_one_procedure;").execute();

// 通过 SQL 查询获取 `@my_var` 的最终值
auto myResult = mySession.sql("SELECT @my_var").execute();

// 获取查询结果的第一行，并打印第一列的值
Row row = myResult.fetchOne();
cout << row[0] << endl;  // 输出 11



/********************** 动态 SQL *****************************/
#include <mysqlx/xdevapi.h>

// Note: The following features are not yet implemented by
// Connector/C++:
// - DataSoure configuration files,
// - quoteName() method.

Table createTestTable(Session &session, const string &name)
{
  string quoted_name = string("`")
                     + session.getDefaultSchemaName()
                     + L"`.`" + name + L"`";
  session.sql(string("DROP TABLE IF EXISTS") + quoted_name).execute();

  string create = "CREATE TABLE ";
  create += quoted_name;
  create += L"(id INT NOT NULL PRIMARY KEY AUTO_INCREMENT)";

  session.sql(create).execute();
  return session.getDefaultSchema().getTable(name);
}

Session session(33060, "user", "password");

Table table1 = createTestTable(session, "test1");
Table table2 = createTestTable(session, "test2");



#include <mysqlx/xdevapi.h>  // 引入 MySQL X DevAPI 头文件
#include <iostream>

using namespace mysqlx;  // 使用 mysqlx 命名空间，简化代码

int main() {
    // 连接到 MySQL 服务器
    Session session("localhost", 33060, "user", "password");

    // 获取数据库对象
    Schema db = session.getSchema("test");

    // -------------------- 方法链式查询 (推荐方式) --------------------
    // 通过 `getTable` 获取 `employee` 表对象
    Table employees = db.getTable("employee");

    // 使用方法链（method chaining）执行 SQL 查询
    RowResult res = employees
        .select("name", "age")  // 选择 `name` 和 `age` 列
        .where("name like :param")  // 添加 WHERE 条件，使用占位符 `:param`
        .orderBy("name")  // 按 `name` 进行排序
        .bind("param", "m%")  // 绑定占位符 `:param`，匹配以 `m` 开头的名字
        .execute();  // 执行查询

    // 遍历查询结果并输出
    std::cout << "Method chaining query results:" << std::endl;
    for (Row row : res) {
        std::cout << "Name: " << row[0] << ", Age: " << row[1] << std::endl;
    }

    // -------------------- 传统 SQL 语句执行（不推荐） --------------------
    // 直接使用 `sql()` 方法执行 SQL 语句（适用于复杂查询）
    RowResult result = session.sql(
        "SELECT name, age "
        "FROM employee "
        "WHERE name like ? "
        "ORDER BY name"
    )
    .bind("m%")  // 绑定 `?` 占位符
    .execute();  // 执行 SQL 语句

    // 遍历查询结果并输出
    std::cout << "\nTraditional SQL query results:" << std::endl;
    for (Row row : result) {
        std::cout << "Name: " << row[0] << ", Age: " << row[1] << std::endl;
    }

    return 0;
}



/********************** 参数绑定 *****************************/
#include <mysqlx/xdevapi.h>  // 引入 MySQL X DevAPI 头文件
#include <iostream>

using namespace mysqlx;  // 使用 mysqlx 命名空间，简化代码

int main() {
    // 连接到 MySQL 服务器
    Session session("localhost", 33060, "user", "password");

    // 获取数据库 "test"
    Schema db = session.getSchema("test");

    // 获取集合（Collection），类似于 MongoDB 的集合
    Collection myColl = db.getCollection("my_collection");

    // -------------------- 1. 直接使用固定值查询 --------------------
    // 查询 age = 18 的所有文档
    auto myRes1 = myColl.find("age = 18").execute();

    // 遍历查询结果并输出
    std::cout << "Documents where age = 18:" << std::endl;
    for (auto doc : myRes1) {
        std::cout << doc << std::endl;
    }

    // -------------------- 2. 使用 .bind() 绑定参数 --------------------
    // 查询 name = "Rohit" 且 age = 18 的所有文档
    auto myRes2 = myColl.find("name = :param1 AND age = :param2")
                        .bind("param1", "Rohit")  // 绑定参数 param1 = "Rohit"
                        .bind("param2", 18)  // 绑定参数 param2 = 18
                        .execute();

    // 输出查询结果
    std::cout << "\nDocuments where name = 'Rohit' and age = 18:" << std::endl;
    for (auto doc : myRes2) {
        std::cout << doc << std::endl;
    }

    // -------------------- 3. 使用 named parameters 修改数据 --------------------
    // 修改 name = "Nadya" 的文档，将 age 设置为 55
    myColl.modify("name = :param")
          .set("age", 55)  // 设置 age = 55
          .bind("param", "Nadya")  // 绑定参数 param = "Nadya"
          .execute();

    std::cout << "\nUpdated age to 55 where name = 'Nadya'." << std::endl;

    // -------------------- 4. 绑定参数进行模糊查询 --------------------
    // 查询 name 以 "R" 开头的所有文档
    auto myRes3 = myColl.find("name like :param")
                        .bind("param", "R%")  // 绑定参数 param = "R%"
                        .execute();

    // 输出查询结果
    std::cout << "\nDocuments where name starts with 'R':" << std::endl;
    for (auto doc : myRes3) {
        std::cout << doc << std::endl;
    }

    return 0;
}



#include <mysqlx/xdevapi.h>  // 引入 MySQL X DevAPI 头文件
#include <iostream>

using namespace mysqlx;  // 使用 mysqlx 命名空间，简化代码

int main() {
    try {
        // -------------------- 1. 连接到 MySQL 服务器 --------------------
        // 连接到本地 MySQL 服务器，端口 33060，使用指定的用户名和密码
        Session session(33060, "user", "password");

        // 获取 "test" 数据库
        Schema db = session.getSchema("test");

        // -------------------- 2. 创建一个新的集合 my_collection --------------------
        // 如果 "my_collection" 已存在，则删除旧的集合
        if (db.existsInDatabase("my_collection")) {
            db.dropCollection("my_collection");
        }

        // 创建集合 my_collection（类似于 MongoDB 的 Collection）
        Collection myColl = db.createCollection("my_collection");

        // -------------------- 3. 插入文档 --------------------
        // 插入一个 JSON 格式的文档，表示一个人的姓名和年龄
        myColl.add(R"({ "name": "Laurie", "age": 19 })").execute();
        myColl.add(R"({ "name": "Nadya", "age": 54 })").execute();
        myColl.add(R"({ "name": "Lukas", "age": 32 })").execute();

        std::cout << "Inserted documents successfully." << std::endl;

        // -------------------- 4. 查询文档 --------------------
        // 查找 name 以 'L' 开头并且 age 小于 20 的文档
        DocResult docs = myColl.find("name like :param1 AND age < :param2")
                               .limit(1)  // 限制返回 1 条数据
                               .bind("param1", "L%")  // 绑定参数 param1 = "L%"，即 name 以 L 开头
                               .bind("param2", 20)  // 绑定参数 param2 = 20，即 age < 20
                               .execute();

        // -------------------- 5. 打印查询结果 --------------------
        // fetchOne() 取出第一条匹配的文档
        std::cout << "Found document: " << docs.fetchOne() << std::endl;

        // -------------------- 6. 删除集合 --------------------
        db.dropCollection("my_collection");
        std::cout << "Collection 'my_collection' deleted." << std::endl;

    } catch (const mysqlx::Error &err) {
        std::cerr << "MySQL Error: " << err.what() << std::endl;
    } catch (std::exception &ex) {
        std::cerr << "Standard Exception: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown Exception occurred!" << std::endl;
    }

    return 0;
}



#include <mysqlx/xdevapi.h>  // MySQL X DevAPI 头文件
#include <iostream>  // 用于 std::cout 和 std::endl

using namespace mysqlx;
using namespace std;

int main() {
    try {
        // -------------------- 1. 连接到 MySQL 服务器 --------------------
        Session mySession(33060, "user", "password");

        // 获取 "test" 数据库
        Schema myDb = mySession.getSchema("test");

        // -------------------- 2. 访问数据库表 my_table --------------------
        Table myTable = myDb.getTable("my_table");

        // -------------------- 3. 插入数据 --------------------
        myTable.insert("name", "birthday", "age")
               .values("Laurie", "2000-05-27", 19)
               .execute();

        cout << "Inserted data successfully." << endl;

        // -------------------- 4. 查询数据 --------------------
        RowResult myResult = myTable.select("_id", "name", "birthday")
            .where("name like :name AND age < :age")
            .bind("name", "L%")  // 绑定参数 name = "L%"
            .bind("age", 30)     // 绑定参数 age < 30
            .execute();

        // -------------------- 5. 获取查询结果 --------------------
        Row row = myResult.fetchOne();  // 获取第一条数据
        if (!row) {
            cout << "No matching data found." << endl;
        } else {
            cout << "     _id: " << row[0] << endl;
            cout << "    name: " << row[1] << endl;
            cout << "birthday: " << row[2] << endl;
        }

    } catch (const mysqlx::Error &err) {
        cerr << "MySQL Error: " << err.what() << endl;
    } catch (const std::exception &ex) {
        cerr << "Standard Exception: " << ex.what() << endl;
    } catch (...) {
        cerr << "Unknown Exception occurred!" << endl;
    }

    return 0;
}
