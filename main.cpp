#include "httplib.h"
#include <map>
#include <nlohmann/json.hpp>
#include <mysql/mysql.h>
#include <iostream>
#include "client.pb.h"
#include <brpc/channel.h>
int main() {
    // 创建Channel对象，用于与服务器建立连接
    brpc::Channel channel;
    if (channel.Init("localhost:9000", nullptr) != 0) {
        std::cerr << "无法连接到服务器" << std::endl;
        return -1;
    }

    // 创建Stub对象，用于向服务器发送RPC请求
    client::proto::CLIENTService_Stub stub(&channel);

    httplib::Server svr;
    MYSQL* conn = mysql_init(NULL);
    if (conn == NULL) {
        // 处理 MySQL 连接初始化错误
        // ...
        return 1;
    }

    // 连接到 MySQL 数据库
    if (mysql_real_connect(conn, "localhost", "root", "new_password", "my_database", 0, NULL, 0) == NULL) {
        // 处理 MySQL 连接错误
        return 1;
    }
    // 处理根路径请求，返回网页
    svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        std::ifstream file("/home/user/CLionProjects/sql/index.html");
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            res.set_content(content, "text/html");
        } else {
            res.set_content("Error: Unable to open the HTML file", "text/plain");
        }
    });

    // 处理插入数据请求
    svr.Post("/insert_data", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json json_data = nlohmann::json::parse(req.body);
        std::string key = json_data["key"];
        std::string value = json_data["value"];

        // 构造插入数据的 SQL 语句
        std::string insert_query = "INSERT INTO data_table (key_column, value_column) VALUES ('" + key + "', '" + value + "')";

        // 执行插入数据的 SQL 语句
        mysql_query(conn, insert_query.c_str()) ;
        // 构造Insert请求消息
        client::proto::InsertRequest insert_request;
        insert_request.set_key(key); // 设置插入数据的键
        insert_request.set_value(value); // 设置插入数据的值

        // 发送InsertData RPC请求
        client::proto::InsertResponse insert_response;
        brpc::Controller controller;
        stub.InsertData(&controller, &insert_request, &insert_response, nullptr);

        // 检查InsertData请求是否成功
        if (controller.Failed()) {
            std::cerr << "InsertData请求失败: " << controller.ErrorText() << std::endl;
        } else {
            std::cout << "InsertData请求成功，返回结果: " << insert_response.success() << std::endl;
        }
        nlohmann::json result = {{"result", "Data inserted successfully"}};
        res.set_content(result.dump(), "application/json");
    });

    // 处理查询数据请求
    svr.Get("/search_data", [&](const httplib::Request& req, httplib::Response& res) {
        std::string key = req.get_param_value("key");
        // 构造查询数据的 SQL 语句
        std::string select_query = "SELECT value_column FROM data_table WHERE key_column='" + key + "'";

        // 执行查询数据的 SQL 语句
        mysql_query(conn, select_query.c_str()) ;

        // 获取查询结果
        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content("query error", "text/plain");
            // ...
            return;
        }

        // 获取查询结果的行数
        int num_rows = mysql_num_rows(result);
        if (num_rows == 0) {
            res.set_content("Key not found", "text/plain");
        } else {
            // 获取查询结果的第一行数据
            MYSQL_ROW row = mysql_fetch_row(result);
            std::string value = row[0];
            // 构造Query请求消息
            client::proto::QueryRequest query_request;
            query_request.set_key(key); //
            query_request.set_value(value); // 设置查询数据的键


            // 发送QueryData RPC请求
            client::proto::QueryResponse query_response;
            brpc::Controller controller;
            stub.QueryData(&controller, &query_request, &query_response, nullptr);

            // 检查QueryData请求是否成功
            if (controller.Failed()) {
                std::cerr << "QueryData请求失败: " << controller.ErrorText() << std::endl;
            } else {
                std::cout << "QueryData请求成功，返回结果: " << query_response.message() << std::endl;
            }
            res.set_content(query_response.message() , "text/plain");
        }

        // 释放查询结果
        mysql_free_result(result);

    });
    svr.listen("0.0.0.0", 8080);
    return 0;
}

