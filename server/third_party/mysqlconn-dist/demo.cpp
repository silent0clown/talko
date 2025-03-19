#include <mysqlx/xdevapi.h>
#include <iostream>

int main() {
    try {
        // 1. 创建会话
        mysqlx::Session session("127.0.0.1", 66030, "root", "talko_root");

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

// #include <iostream>
// #include <mysqlx/xdevapi.h>

// using ::std::cout;
// using ::std::endl;
// using namespace ::mysqlx;


// int main(int argc, const char* argv[])
// try {

//   const char   *url = (argc > 1 ? argv[1] : "mysqlx://root@127.0.0.1");

//   cout << "Creating session on " << url
//        << " ..." << endl;

//   Session sess(url);

//   cout <<"Session accepted, creating collection..." <<endl;

//   Schema sch= sess.getSchema("test");
//   Collection coll= sch.createCollection("c1", true);

//   cout <<"Inserting documents..." <<endl;

//   coll.remove("true").execute();

//   {
//     DbDoc doc(R"({ "name": "foo", "age": 1 })");

//     Result add =
//       coll.add(doc)
//           .add(R"({ "name": "bar", "age": 2, "toys": [ "car", "ball" ] })")
//           .add(R"({ "name": "bar", "age": 2, "toys": [ "car", "ball" ] })")
//           .add(R"({
//                  "name": "baz",
//                   "age": 3,
//                  "date": { "day": 20, "month": "Apr" }
//               })")
//           .add(R"({ "_id": "myuuid-1", "name": "foo", "age": 7 })")
//           .execute();

//     std::list<string> ids = add.getGeneratedIds();
//     for (string id : ids)
//       cout <<"- added doc with id: " << id <<endl;
//   }

//   cout <<"Fetching documents..." <<endl;

//   DocResult docs = coll.find("age > 1 and name like 'ba%'").execute();

//   int i = 0;
//   for (DbDoc doc : docs)
//   {
//     cout <<"doc#" <<i++ <<": " <<doc <<endl;

//     for (Field fld : doc)
//     {
//       cout << " field `" << fld << "`: " <<doc[fld] << endl;
//     }

//     string name = doc["name"];
//     cout << " name: " << name << endl;

//     if (doc.hasField("date") && Value::DOCUMENT == doc.fieldType("date"))
//     {
//       cout << "- date field" << endl;
//       DbDoc date = doc["date"];
//       for (Field fld : date)
//       {
//         cout << "  date `" << fld << "`: " << date[fld] << endl;
//       }
//       string month = doc["date"]["month"];
//       int day = date["day"];
//       cout << "  month: " << month << endl;
//       cout << "  day: " << day << endl;
//     }

//     if (doc.hasField("toys") && Value::ARRAY == doc.fieldType("toys"))
//     {
//       cout << "- toys:" << endl;
//       for (auto toy : doc["toys"])
//       {
//         cout << "  " << toy << endl;
//       }
//     }

//     cout << endl;
//   }
//   cout <<"Done!" <<endl;
// }
// catch (const mysqlx::Error &err)
// {
//   cout <<"ERROR: " <<err <<endl;
//   return 1;
// }
// catch (std::exception &ex)
// {
//   cout <<"STD EXCEPTION: " <<ex.what() <<endl;
//   return 1;
// }
// catch (const char *ex)
// {
//   cout <<"EXCEPTION: " <<ex <<endl;
//   return 1;
// }

