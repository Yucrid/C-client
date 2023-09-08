#include <iostream>
#include <httplib.h>
#include <mysql/mysql.h>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <openssl/sha.h>
#include <crypto.h>
#include "glog/logging.h"
using json = nlohmann::json;
MYSQL *conn;
using HashString = util::OpenSSLSHA256::digestType;
HashString HashFunc(const HashString &h1, const HashString &h2) {
    util::OpenSSLSHA256 hash;
    if (!hash.update(h1.data(), h1.size()) || !hash.update(h2.data(), h2.size())) {
        CHECK(false) << "Can not update digest.";
    }
    auto digest = hash.final();
    if (digest == std::nullopt) {
        CHECK(false) << "Can not generate digest.";
    }
    return *digest;
}

HashString calculateSHA256(const std::string &input) {
    HashString hash;
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();

    if (mdctx == nullptr) {
        // 处理错误
        // ...
    }

    if (EVP_DigestInit(mdctx, EVP_sha256()) != 1) {
        // 处理初始化错误
        // ...
    }

    if (EVP_DigestUpdate(mdctx, input.c_str(), input.length()) != 1) {
        // 处理更新错误
        // ...
    }

    if (EVP_DigestFinal(mdctx, hash.data(), nullptr) != 1) {
        // 处理最终哈希计算错误
        // ...
    }

    EVP_MD_CTX_free(mdctx);
    return hash;
}
HashString combineHashValues(const HashString &hash1, const HashString &hash2) {
    HashString combinedHash;
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();

    if (mdctx == nullptr) {
        // 处理错误
        // ...
    }

    if (EVP_DigestInit(mdctx, EVP_sha256()) != 1) {
        // 处理初始化错误
        // ...
    }

    if (EVP_DigestUpdate(mdctx, hash1.data(), hash1.size()) != 1) {
        // 处理第一个哈希值更新错误
        // ...
    }

    if (EVP_DigestUpdate(mdctx, hash2.data(), hash2.size()) != 1) {
        // 处理第二个哈希值更新错误
        // ...
    }

    if (EVP_DigestFinal(mdctx, combinedHash.data(), nullptr) != 1) {
        // 处理最终哈希计算错误
        // ...
    }

    EVP_MD_CTX_free(mdctx);
    return combinedHash;
}
void handle_post(const httplib::Request &req, httplib::Response &res) {

    json body;
    try {
        body = json::parse(req.body);
    } catch (const json::parse_error &e) {
        res.status = 400;
        res.set_content("Invalid JSON data", "text/plain");
        return;
    }

    if (body.contains("key") && body.contains("value")) {
        std::string key = body["key"];
        std::string value = body["value"];

        std::string message = "You entered Key: " + key + " and Value: " + value;


        // 构造插入数据的 SQL 语句
        std::string insert_query = "INSERT INTO data_table (key_column, value_column) VALUES ('" + key + "', '" + value + "')";

        // 执行插入数据的 SQL 语句
        mysql_query(conn, insert_query.c_str()) ;

        json response;
        response["success"] = true;
        response["message"] = message;

        res.set_content(response.dump(), "application/json");
    } else {
        json response;
        response["success"] = false;
        response["message"] = "Invalid request. Missing key or value.";
        res.status = 400;
        res.set_content(response.dump(), "application/json");
    }
}
void handle_change_post(const httplib::Request &req, httplib::Response &res) {

    json body;
    try {
        body = json::parse(req.body);
    } catch (const json::parse_error &e) {
        res.status = 400;
        res.set_content("Invalid JSON data", "text/plain");
        return;
    }

    if (body.contains("key") && body.contains("value")) {
        std::string key = body["key"];
        std::string value = body["value"];

        std::string message = "Key: " + key + " new Value: " + value;

        // 构造插入数据的 SQL 语句
        std::string update_query = "UPDATE data_table SET value_column = '" + value + "' WHERE key_column = '" + key + "'";

        if (mysql_query(conn, update_query.c_str()) == 0) {
            json response;
            response["success"] = true;
            response["message"] = message;
            res.set_content(response.dump(), "application/json");
        } else {
            json response;
            response["success"] = false;
            response["message"] = "Failed to update data.";
            res.status = 500;
            res.set_content(response.dump(), "application/json");
        }
    } else {
        json response;
        response["success"] = false;
        response["message"] = "Invalid request. Missing key or value.";
        res.status = 400;
        res.set_content(response.dump(), "application/json");
    }
}
void handle_get(const httplib::Request &req, httplib::Response &res) {
    // 获取URL中的查询参数
    json body;
    try {
        body = json::parse(req.body);
    } catch (const json::parse_error &e) {
        res.status = 400;
        res.set_content("Invalid JSON data", "text/plain");
        return;
    }

    if (body.contains("key") && body.contains("message")) {
        // 构造查询数据的 SQL 语句
        std::string key = body["key"];
        std::string message = body["message"];
        std::string select_query = "SELECT value_column FROM data_table WHERE key_column = '" + key + "'";
        // 执行查询数据的 SQL 语句
        if (mysql_query(conn, select_query.c_str()) == 0) {
            MYSQL_RES *result = mysql_store_result(conn);
            if (result != nullptr) {
                // 获取查询结果
                MYSQL_ROW row = mysql_fetch_row(result);
                if (row != nullptr) {
                    // 提取查询结果中的value值
                    std::string value = row[0];
                    json jsonData = json::parse(message);

                    // 访问嵌套的属性
                    std::string str = jsonData["digest"];
                    HashString hash = jsonData["hash"];
                    HashString root = jsonData["root"];
                    std::vector<HashString> proof = jsonData["merkleProof_Siblings"];
                    uint32_t path = jsonData["merkleProof_Path"];
                    HashString valueHash = calculateSHA256(value);
                    //HashString digest = calculateSHA256(str);
                    for (int i=0 ; i<proof.size(); i++) {
                        if ((path & 1) == 1) {  // hash leaf 1 before leaf 0
                            hash = combineHashValues(hash, proof[i]);
                        } else {
                            hash = combineHashValues(proof[i], hash);
                        }
                        path >>= 1;
                    }
                    // 打印提取的数据
                    json response;
                    response["success"] = true;
                    response["message"] = proof;
                    response["root"] = root;
                    response["digest"] = str;
                    response["valuehash"] = valueHash;
                    response["hash"] = hash;
                    res.set_content(response.dump(), "application/json");
                    mysql_free_result(result);
                    return;
                }
                mysql_free_result(result);
            }
        }

        // 如果查询失败或没有找到对应的value值，返回错误的JSON响应
        json response;
        response["success"] = false;
        response["message"] = "Failed to find value for key: " + key;
        res.status = 404;
        res.set_content(response.dump(), "application/json");
    } else {
        json response;
        response["success"] = false;
        response["message"] = "Invalid request. Missing key.";
        res.status = 400;
        res.set_content(response.dump(), "application/json");
    }
}
int main() {
    httplib::Server server;
    conn = mysql_init(nullptr);
    if (conn == nullptr) {
        std::cerr << "Failed to initialize MySQL" << std::endl;
        return 1;
    }

    // Connect to the MySQL database
    if (mysql_real_connect(conn, "localhost", "root", "new_password", "my_database", 0,NULL, 0) == nullptr) {
        std::cerr << "Failed to connect to MySQL database" << std::endl;
        mysql_close(conn);
        return 1;
    }

    server.Post("/api/submit-data", handle_post);
    server.Post("/api/change-data", handle_change_post);
    server.Post("/api/get-data", handle_get);

    if (server.listen("0.0.0.0", 8090)) {
        std::cout << "Server started at http://localhost:8090/" << std::endl;
    } else {
        std::cerr << "Failed to start the server" << std::endl;
    }

    return 0;
}

