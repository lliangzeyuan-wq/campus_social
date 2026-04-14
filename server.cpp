#include <iostream>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <openssl/md5.h>
#include <mysql/mysql.h>
#include<nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

const int PORT = 8080;
const int BUF_SIZE = 4096;

// MySQL 配置
#define DB_HOST "localhost"
#define DB_USER "root"
#define DB_PASS "Ll19299637611@"
#define DB_NAME "partner"

// MD5加密（修复宏名拼写错误）
string md5(const string& str) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((const unsigned char*)str.c_str(), str.size(), digest);
    char md5_str[33];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(&md5_str[i * 2], "%02x", (unsigned int)digest[i]);
    return string(md5_str);
}

// 修复MySQL弃用警告，删除过时的重连选项
MYSQL* createMySQLConn() {
    MYSQL* conn = mysql_init(NULL);
    if (!conn) {
        cerr << "MySQL 初始化失败" << endl;
        return NULL;
    }

    // 设置超时时间
    unsigned int timeout = 30;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 3306, NULL, 0)) {
        cerr << "MySQL 连接失败: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return NULL;
    }
    mysql_set_character_set(conn, "utf8mb4");
    mysql_query(conn,"SET NAMES utf8mb4");
    return conn;
}

// 验证Token，带连接检查
string verifyToken(MYSQL* conn, const string& token) {
    if (token.empty() || !conn || mysql_ping(conn) != 0) return "";

    char escaped_token[1024];
    mysql_real_escape_string(conn, escaped_token, token.c_str(), token.size());

    char sql[512];
    sprintf(sql, "SELECT id FROM user WHERE token='%s'", escaped_token);

    if (mysql_query(conn, sql)) return "";
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return "";

    MYSQL_ROW row = mysql_fetch_row(res);
    string user_id = row ? row[0] : "";
    mysql_free_result(res);
    return user_id;
}

// 初始化MySQL库
bool initMySQL() {
    if (mysql_library_init(0, NULL, NULL)) {
        cerr << "MySQL 库初始化失败" << endl;
        return false;
    }
    cout << "? MySQL 库初始化成功" << endl;
    return true;
}

// 生成Token
string genToken() {
    return "token_" + to_string(time(0)) + "_" + to_string(rand() % 10000);
}

// 处理客户端请求
void* handleClient(void* arg) {
    int client_fd = *(int*)arg;
    delete (int*)arg;
    char buffer[BUF_SIZE] = { 0 };

    // 每个线程独立创建MySQL连接
    MYSQL* conn = createMySQLConn();
    if (!conn) {
        cerr << "线程MySQL连接创建失败" << endl;
        close(client_fd);
        pthread_exit(NULL);
    }

    ssize_t recv_len = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (recv_len <= 0) {
        mysql_close(conn);
        close(client_fd);
        pthread_exit(NULL);
    }

    string request(buffer);
    string method, path, version;
    stringstream req_stream(request);
    req_stream >> method >> path >> version;

    size_t header_end = request.find("\r\n\r\n");
    string body = request.substr(header_end + 4);

    // 跨域响应头
    size_t origin_pos = request.find("Origin: ");
    string origin = "*";
    if (origin_pos != string::npos) {
        origin = request.substr(origin_pos + 8);
        size_t end_pos = origin.find("\r\n");
        if (end_pos != string::npos) origin = origin.substr(0, end_pos);
    }

    string resp_headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Access-Control-Allow-Origin: " + origin + "\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Connection: close\r\n\r\n";

    // 处理OPTIONS预检请求
    if (method == "OPTIONS") {
        string resp = resp_headers;
        send(client_fd, resp.c_str(), resp.size(), 0);
        mysql_close(conn);
        close(client_fd);
        pthread_exit(NULL);
    }

    json resp_json;
    MYSQL_RES* res;
    MYSQL_ROW row;

    // 提取Token
    string token = "";
    size_t auth_pos = request.find("Authorization: Bearer ");
    if (auth_pos != string::npos) {
        token = request.substr(auth_pos + 22);
        size_t end_pos = token.find("\r\n");
        if (end_pos != string::npos) token = token.substr(0, end_pos);
    }

    // 验证当前用户ID
    string uid = verifyToken(conn, token);

    // ===================== 登录接口 =====================
    if (method == "POST" && path == "/api/login") {
        try {
            json req_json = json::parse(body);
            string studentId = req_json["studentId"].get<string>();
            string password = req_json["password"].get<string>();
            string pwd = md5(password);

            char sql[512];
            sprintf(sql, "SELECT id, name, gender FROM user WHERE id='%s' AND password='%s'",
                studentId.c_str(), pwd.c_str());

            if (mysql_query(conn, sql)) {
                resp_json["success"] = false;
                resp_json["message"] = "登录失败：" + string(mysql_error(conn));
            }
            else {
                res = mysql_store_result(conn);
                if ((row = mysql_fetch_row(res))) {
                    string login_token = genToken();
                    char update_sql[512];
                    sprintf(update_sql, "UPDATE user SET token='%s' WHERE id='%s'", login_token.c_str(), studentId.c_str());
                    mysql_query(conn, update_sql);

                    resp_json["success"] = true;
                    resp_json["data"] = {
                        {"id", row[0]},
                        {"studentId", studentId},
                        {"name", row[1]},
                        {"gender", row[2]},
                        {"token", login_token},
                        {"profileCompleted", true}
                    };
                }
                else {
                    resp_json["success"] = false;
                    resp_json["message"] = "学号或密码错误";
                }
                mysql_free_result(res);
            }
        }
        catch (...) {
            resp_json["success"] = false;
            resp_json["message"] = "参数错误";
        }
    }
    // ===================== 注册接口 =====================
    else if (method == "POST" && path == "/api/register") {
        try {
            json req_json = json::parse(body);
            string studentId = req_json["studentId"].get<string>();
            string name = req_json["name"].get<string>();
            string gender = req_json["gender"].get<string>();
            string password = req_json["password"].get<string>();
            string pwd = md5(password);

            // 检查学号是否存在
            char check_sql[512];
            sprintf(check_sql, "SELECT id FROM user WHERE id='%s'", studentId.c_str());
            if (mysql_query(conn, check_sql) == 0) {
                MYSQL_RES* check_res = mysql_store_result(conn);
                if (mysql_num_rows(check_res) > 0) {
                    mysql_free_result(check_res);
                    resp_json["success"] = false;
                    resp_json["message"] = "注册失败（学号已存在）";
                    string resp = resp_headers + resp_json.dump();
                    send(client_fd, resp.c_str(), resp.size(), 0);
                    mysql_close(conn);
                    close(client_fd);
                    pthread_exit(NULL);
                }
                mysql_free_result(check_res);
            }

            // 插入用户
            char sql[1024];
            sprintf(sql, "INSERT INTO user (id, name, gender, password) VALUES ('%s','%s','%s','%s')",
                studentId.c_str(), name.c_str(), gender.c_str(), pwd.c_str());

            if (mysql_query(conn, sql)) {
                resp_json["success"] = false;
                resp_json["message"] = "注册失败：" + string(mysql_error(conn));
            }
            else {
                resp_json["success"] = true;
                resp_json["message"] = "注册成功";
            }
        }
        catch (...) {
            resp_json["success"] = false;
            resp_json["message"] = "参数错误";
        }
    }
    // ===================== 匹配列表 =====================
    else if (method == "GET" && path.find("/api/matches") == 0) {
        if (uid.empty()) {
            resp_json["success"] = false; resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        vector<json> users;
        char escaped_uid[256];
        mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());

        char sql[1024];
        sprintf(sql, "SELECT id, name, gender, study_grade, postgrad_major, postgrad_school, friend_hobby, current_mode FROM user WHERE id!='%s'", escaped_uid);

        if (mysql_query(conn, sql)) {
            resp_json["success"] = false;
            resp_json["message"] = "查询失败：" + string(mysql_error(conn));
        }
        else {
            res = mysql_store_result(conn);
            while ((row = mysql_fetch_row(res))) {
                users.push_back({
                    {"id", row[0] ? row[0] : ""},
                    {"name", row[1] ? row[1] : ""},
                    {"gender", row[2] ? row[2] : ""},
                    {"grade", row[3] ? row[3] : "--"},
                    {"major", row[4] ? row[4] : "--"},
                    {"college", row[5] ? row[5] : "--"},
                    {"interests", row[6] ? row[6] : "--"},
                    {"studyTime", row[7] ? row[7] : "--"},
                    {"matchScore", 85 + rand() % 15}
                    });
            }
            mysql_free_result(res);
            resp_json["success"] = true;
            resp_json["data"] = users;
        }
    }
    // ===================== 个人资料查询接口 =====================
    else if (method == "GET" && path.substr(0, 13) == "/api/profile/") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            resp_json["data"] = nullptr;
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        string user_id = path.substr(13);
        char sql[1024];
        sprintf(sql, "SELECT id, name, gender, study_grade, postgrad_major, postgrad_school, friend_hobby, current_mode, love_intro, location FROM user WHERE id='%s'", user_id.c_str());

        if (mysql_query(conn, sql)) {
            resp_json["success"] = false;
            resp_json["message"] = "查询失败：" + string(mysql_error(conn));
            resp_json["data"] = nullptr;
        }
        else {
            res = mysql_store_result(conn);
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row) {
                resp_json["success"] = true;
                resp_json["data"] = {
                    {"id", row[0] ? row[0] : ""},
                    {"name", row[1] ? row[1] : ""},
                    {"gender", row[2] ? row[2] : "--"},
                    {"grade", row[3] ? row[3] : "--"},
                    {"major", row[4] ? row[4] : "--"},
                    {"college", row[5] ? row[5] : "--"},
                    {"interests", row[6] ? row[6] : "--"},
                    {"studyTime", row[7] ? row[7] : "--"},
                    {"introduction", row[8] ? row[8] : "--"},
                    {"location", row[9] ? row[9] : "--"}
                };
            }
            else {
                resp_json["success"] = false;
                resp_json["message"] = "用户不存在";
                resp_json["data"] = nullptr;
            }
            mysql_free_result(res);
        }
    }
    // ===================== 更新资料接口 =====================
    else if (method == "POST" && path == "/api/profile/update") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        try {
            json req_json = json::parse(body);
            vector<string> update_sql_parts;

            if (req_json.contains("name") && req_json["name"].is_string()) {
                string val = req_json["name"].get<string>();
                char escaped[256];
                mysql_real_escape_string(conn, escaped, val.c_str(), val.size());
                update_sql_parts.push_back("name='" + string(escaped) + "'");
            }
            if (req_json.contains("gender") && req_json["gender"].is_string()) {
                string val = req_json["gender"].get<string>();
                char escaped[32];
                mysql_real_escape_string(conn, escaped, val.c_str(), val.size());
                update_sql_parts.push_back("gender='" + string(escaped) + "'");
            }
            if (req_json.contains("grade") && req_json["grade"].is_string()) {
                string val = req_json["grade"].get<string>();
                char escaped[64];
                mysql_real_escape_string(conn, escaped, val.c_str(), val.size());
                update_sql_parts.push_back("study_grade='" + string(escaped) + "'");
            }
            if (req_json.contains("major") && req_json["major"].is_string()) {
                string val = req_json["major"].get<string>();
                char escaped[256];
                mysql_real_escape_string(conn, escaped, val.c_str(), val.size());
                update_sql_parts.push_back("postgrad_major='" + string(escaped) + "'");
            }
            if (req_json.contains("college") && req_json["college"].is_string()) {
                string val = req_json["college"].get<string>();
                char escaped[256];
                mysql_real_escape_string(conn, escaped, val.c_str(), val.size());
                update_sql_parts.push_back("postgrad_school='" + string(escaped) + "'");
            }
            if (req_json.contains("studyTime") && req_json["studyTime"].is_string()) {
                string val = req_json["studyTime"].get<string>();
                char escaped[128];
                mysql_real_escape_string(conn, escaped, val.c_str(), val.size());
                update_sql_parts.push_back("current_mode='" + string(escaped) + "'");
            }
            if (req_json.contains("introduction") && req_json["introduction"].is_string()) {
                string val = req_json["introduction"].get<string>();
                char escaped[200];
                mysql_real_escape_string(conn, escaped, val.c_str(), val.size());
                update_sql_parts.push_back("love_intro='" + string(escaped) + "'");
            }
            if (req_json.contains("location") && req_json["location"].is_string()) {
                string val = req_json["location"].get<string>();
                char escaped[256];
                mysql_real_escape_string(conn, escaped, val.c_str(), val.size());
                update_sql_parts.push_back("location='" + string(escaped) + "'");
            }
            if (req_json.contains("interests")) {
                string interests_str;
                if (req_json["interests"].is_array()) {
                    vector<string> tags = req_json["interests"].get<vector<string>>();
                    for (int i = 0; i < tags.size(); i++) {
                        if (i > 0) interests_str += ",";
                        interests_str += tags[i];
                    }
                }
                else if (req_json["interests"].is_string()) {
                    interests_str = req_json["interests"].get<string>();
                }
                if (!interests_str.empty()) {
                    char escaped[100];
                    mysql_real_escape_string(conn, escaped, interests_str.c_str(), interests_str.size());
                    update_sql_parts.push_back("friend_hobby='" + string(escaped) + "'");
                }
            }

            if (update_sql_parts.empty()) {
                resp_json["success"] = true;
                resp_json["message"] = "没有需要更新的内容";
            }
            else {
                string sql = "UPDATE user SET ";
                for (int i = 0; i < update_sql_parts.size(); i++) {
                    if (i > 0) sql += ", ";
                    sql += update_sql_parts[i];
                }
                sql += " WHERE id='" + uid + "'";

                if (mysql_query(conn, sql.c_str())) {
                    string err = string(mysql_error(conn));
                    resp_json["success"] = false;
                    resp_json["message"] = "更新失败: " + err;
                }
                else {
                    resp_json["success"] = true;
                    resp_json["message"] = "资料更新成功！";
                }
            }
        }
        catch (const exception& e) {
            resp_json["success"] = false;
            resp_json["message"] = "参数错误: " + string(e.what());
        }
    }
    // ===================== 发起好友邀请接口 =====================
    else if (method == "POST" && path == "/api/invites") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        try {
            json req_json = json::parse(body);
            string target_user_id = req_json["userId"].get<string>();
            string type = req_json.value("type", "study");

            if (target_user_id.empty() || target_user_id == uid) {
                resp_json["success"] = false;
                resp_json["message"] = "参数错误（不能邀请自己）";
                string resp = resp_headers + resp_json.dump();
                send(client_fd, resp.c_str(), resp.size(), 0);
                mysql_close(conn);
                close(client_fd);
                pthread_exit(NULL);
            }

            char escaped_uid[256], escaped_target[256];
            mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());
            mysql_real_escape_string(conn, escaped_target, target_user_id.c_str(), target_user_id.size());

            char check_sql[1024];
            sprintf(check_sql, "SELECT id FROM invitation WHERE from_id='%s' AND to_id='%s' AND status=0",
                escaped_uid, escaped_target);
            if (mysql_query(conn, check_sql) == 0) {
                MYSQL_RES* check_res = mysql_store_result(conn);
                if (mysql_num_rows(check_res) > 0) {
                    mysql_free_result(check_res);
                    resp_json["success"] = false;
                    resp_json["message"] = "您已经发过邀请了，请等待对方处理";
                    string resp = resp_headers + resp_json.dump();
                    send(client_fd, resp.c_str(), resp.size(), 0);
                    mysql_close(conn);
                    close(client_fd);
                    pthread_exit(NULL);
                }
                mysql_free_result(check_res);
            }

            char sql[1024];
            sprintf(sql, "INSERT INTO invitation (from_id, to_id, status) VALUES ('%s', '%s', 0)",
                escaped_uid, escaped_target);

            if (mysql_query(conn, sql)) {
                resp_json["success"] = false;
                resp_json["message"] = "发送失败：" + string(mysql_error(conn));
            }
            else {
                resp_json["success"] = true;
                resp_json["message"] = "邀请发送成功！等待对方接受";
            }
        }
        catch (const exception& e) {
            resp_json["success"] = false;
            resp_json["message"] = "参数错误：" + string(e.what());
        }
    }
    // ===================== 收到的好友申请列表 =====================
    else if (method == "GET" && path == "/api/invites/received") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        vector<json> requests;
        char escaped_uid[256];
        mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());

        char sql[1024];
        sprintf(sql, "SELECT i.id, i.from_id, u.name, i.create_time FROM invitation i LEFT JOIN user u ON i.from_id=u.id WHERE i.to_id='%s' AND i.status=0", escaped_uid);

        if (mysql_query(conn, sql)) {
            resp_json["success"] = false;
            resp_json["message"] = "查询失败：" + string(mysql_error(conn));
        }
        else {
            MYSQL_RES* res = mysql_store_result(conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                requests.push_back({
                    {"id", row[0] ? row[0] : ""},
                    {"inviteId", row[0] ? row[0] : ""},
                    {"fromId", row[1] ? row[1] : ""},
                    {"fromName", row[2] ? row[2] : "未知用户"},
                    {"time", row[3] ? row[3] : ""}
                    });
            }
            mysql_free_result(res);
            resp_json["success"] = true;
            resp_json["data"] = requests;
        }
    }
    // ===================== 发出的好友申请列表 =====================
    else if (method == "GET" && path == "/api/invites/sent") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        vector<json> requests;
        char escaped_uid[256];
        mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());

        char sql[1024];
        sprintf(sql, "SELECT i.id, i.to_id, u.name, i.create_time, i.status FROM invitation i LEFT JOIN user u ON i.to_id=u.id WHERE i.from_id='%s'", escaped_uid);

        if (mysql_query(conn, sql)) {
            resp_json["success"] = false;
            resp_json["message"] = "查询失败：" + string(mysql_error(conn));
        }
        else {
            MYSQL_RES* res = mysql_store_result(conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                string status = "pending";
                if (row[4] && string(row[4]) == "1") status = "accepted";
                if (row[4] && string(row[4]) == "2") status = "rejected";

                requests.push_back({
                    {"id", row[0] ? row[0] : ""},
                    {"toId", row[1] ? row[1] : ""},
                    {"toName", row[2] ? row[2] : "未知用户"},
                    {"time", row[3] ? row[3] : ""},
                    {"status", status}
                    });
            }
            mysql_free_result(res);
            resp_json["success"] = true;
            resp_json["data"] = requests;
        }
    }
    // ===================== 处理好友申请接口 =====================
    else if (method == "POST" && path == "/api/invites/handle") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        try {
            json req_json = json::parse(body);
            string invite_id = req_json["inviteId"].get<string>();
            string action = req_json["action"].get<string>();

            if (invite_id.empty() || (action != "accept" && action != "reject")) {
                resp_json["success"] = false;
                resp_json["message"] = "参数错误";
                string resp = resp_headers + resp_json.dump();
                send(client_fd, resp.c_str(), resp.size(), 0);
                mysql_close(conn);
                close(client_fd);
                pthread_exit(NULL);
            }

            char escaped_uid[256], escaped_invite_id[256];
            mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());
            mysql_real_escape_string(conn, escaped_invite_id, invite_id.c_str(), invite_id.size());

            int status = (action == "accept") ? 1 : 2;

            char select_sql[1024];
            sprintf(select_sql, "SELECT from_id FROM invitation WHERE id='%s' AND to_id='%s' AND status=0", escaped_invite_id, escaped_uid);
            if (mysql_query(conn, select_sql)) {
                resp_json["success"] = false;
                resp_json["message"] = "操作失败：" + string(mysql_error(conn));
                string resp = resp_headers + resp_json.dump();
                send(client_fd, resp.c_str(), resp.size(), 0);
                mysql_close(conn);
                close(client_fd);
                pthread_exit(NULL);
            }

            MYSQL_RES* select_res = mysql_store_result(conn);
            MYSQL_ROW row = mysql_fetch_row(select_res);
            if (!row) {
                mysql_free_result(select_res);
                resp_json["success"] = false;
                resp_json["message"] = "申请不存在或已处理";
                string resp = resp_headers + resp_json.dump();
                send(client_fd, resp.c_str(), resp.size(), 0);
                mysql_close(conn);
                close(client_fd);
                pthread_exit(NULL);
            }

            string from_user_id = row[0] ? row[0] : "";
            mysql_free_result(select_res);

            char update_sql[1024];
            sprintf(update_sql, "UPDATE invitation SET status=%d WHERE id='%s' AND to_id='%s'", status, escaped_invite_id, escaped_uid);
            if (mysql_query(conn, update_sql)) {
                resp_json["success"] = false;
                resp_json["message"] = "操作失败：" + string(mysql_error(conn));
                string resp = resp_headers + resp_json.dump();
                send(client_fd, resp.c_str(), resp.size(), 0);
                mysql_close(conn);
                close(client_fd);
                pthread_exit(NULL);
            }

            if (action == "accept") {
                char friend_sql[1024];
                sprintf(friend_sql, "INSERT INTO friend_relation (user_id_1, user_id_2) VALUES ('%s', '%s')",
                    escaped_uid, from_user_id.c_str());
                mysql_query(conn, friend_sql);
            }

            resp_json["success"] = true;
            resp_json["message"] = (action == "accept") ? "已接受邀请，开始聊天吧！" : "已拒绝邀请";
        }
        catch (const exception& e) {
            resp_json["success"] = false;
            resp_json["message"] = "参数错误：" + string(e.what());
        }
    }
    // ===================== 我的好友列表接口（优化：显示最新消息） =====================
    else if (method == "GET" && path == "/api/chats") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        vector<json> friends;
        char escaped_uid[256];
        mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());

        // 优化：查询每个好友的最新一条聊天消息
        char sql[2048];
        sprintf(sql,
            "SELECT u.id, u.name, "
            "(SELECT content FROM chat_message "
            "WHERE (from_id='%s' AND to_id=u.id) OR (from_id=u.id AND to_id='%s') "
            "ORDER BY create_time DESC LIMIT 1) AS last_msg "
            "FROM friend_relation fr "
            "LEFT JOIN user u ON (fr.user_id_1=u.id OR fr.user_id_2=u.id) "
            "WHERE (fr.user_id_1='%s' OR fr.user_id_2='%s') AND u.id!='%s'",
            escaped_uid, escaped_uid, escaped_uid, escaped_uid, escaped_uid);

        if (mysql_query(conn, sql)) {
            resp_json["success"] = false;
            resp_json["message"] = "查询失败：" + string(mysql_error(conn));
        }
        else {
            MYSQL_RES* res = mysql_store_result(conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                friends.push_back({
                    {"userId", row[0] ? row[0] : ""},
                    {"name", row[1] ? row[1] : "好友"},
                    {"lastMessage", row[2] ? row[2] : "已成为好友"}
                    });
            }
            mysql_free_result(res);
            resp_json["success"] = true;
            resp_json["data"] = friends;
        }
    }
    // ===================== 聊天记录列表（已修复截取长度） =====================
    else if (method == "GET" && path.substr(0, 14) == "/api/messages/") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        string target_user_id = path.substr(14);
        vector<json> messages;
        char escaped_uid[256], escaped_target[256];
        mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());
        mysql_real_escape_string(conn, escaped_target, target_user_id.c_str(), target_user_id.size());

        char sql[1024];
        sprintf(sql, "SELECT from_id, content, create_time FROM chat_message WHERE (from_id='%s' AND to_id='%s') OR (from_id='%s' AND to_id='%s') ORDER BY create_time ASC",
            escaped_uid, escaped_target, escaped_target, escaped_uid);

        if (mysql_query(conn, sql)) {
            resp_json["success"] = false;
            resp_json["message"] = "查询失败：" + string(mysql_error(conn));
        }
        else {
            MYSQL_RES* res = mysql_store_result(conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                messages.push_back({
                    {"fromId", row[0] ? row[0] : ""},
                    {"from_id", row[0] ? row[0] : ""},
                    {"content", row[1] ? row[1] : ""},
                    {"time", row[2] ? row[2] : ""},
                    {"send_time", row[2] ? row[2] : ""}
                    });
            }
            mysql_free_result(res);
            resp_json["success"] = true;
            resp_json["data"] = messages;
        }
    }
    // ===================== 发送消息接口 =====================
    else if (method == "POST" && path == "/api/messages") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
            string resp = resp_headers + resp_json.dump();
            send(client_fd, resp.c_str(), resp.size(), 0);
            mysql_close(conn);
            close(client_fd);
            pthread_exit(NULL);
        }

        try {
            json req_json = json::parse(body);
            string target_user_id = req_json["userId"].get<string>();
            string content = req_json["content"].get<string>();

            if (target_user_id.empty() || content.empty()) {
                resp_json["success"] = false;
                resp_json["message"] = "参数错误";
                string resp = resp_headers + resp_json.dump();
                send(client_fd, resp.c_str(), resp.size(), 0);
                mysql_close(conn);
                close(client_fd);
                pthread_exit(NULL);
            }

            char escaped_uid[256], escaped_target[256], escaped_content[2048];
            mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());
            mysql_real_escape_string(conn, escaped_target, target_user_id.c_str(), target_user_id.size());
            mysql_real_escape_string(conn, escaped_content, content.c_str(), content.size());

            char sql[2048];
            sprintf(sql, "INSERT INTO chat_message (from_id, to_id, content) VALUES ('%s', '%s', '%s')",
                escaped_uid, escaped_target, escaped_content);

            if (mysql_query(conn, sql)) {
                resp_json["success"] = false;
                resp_json["message"] = "发送失败：" + string(mysql_error(conn));
            }
            else {
                resp_json["success"] = true;
                resp_json["message"] = "发送成功";
                resp_json["data"] = { {"messageId", to_string(mysql_insert_id(conn))} };
            }
        }
        catch (const exception& e) {
            resp_json["success"] = false;
            resp_json["message"] = "参数错误：" + string(e.what());
        }
    }
    // ===================== 统计数据接口（修复：真实统计数据） =====================
    else if (method == "GET" && path == "/api/stats") {
        if (uid.empty()) {
            resp_json["success"] = false;
            resp_json["message"] = "请先登录";
        }
        else {
            char escaped_uid[256];
            mysql_real_escape_string(conn, escaped_uid, uid.c_str(), uid.size());

            // 统计好友数量
            int friend_count = 0;
            char friend_sql[512];
            sprintf(friend_sql, "SELECT COUNT(*) FROM friend_relation WHERE user_id_1='%s' OR user_id_2='%s'", escaped_uid, escaped_uid);
            if (mysql_query(conn, friend_sql) == 0) {
                MYSQL_RES* res = mysql_store_result(conn);
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row) friend_count = atoi(row[0]);
                mysql_free_result(res);
            }

            // 统计匹配成功数量
            int match_count = 0;
            char match_sql[512];
            sprintf(match_sql, "SELECT COUNT(*) FROM invitation WHERE from_id='%s' AND status=1", escaped_uid);
            if (mysql_query(conn, match_sql) == 0) {
                MYSQL_RES* res = mysql_store_result(conn);
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row) match_count = atoi(row[0]);
                mysql_free_result(res);
            }

            // 统计今日聊天消息数
            int chat_count = 0;
            time_t now = time(NULL);
            struct tm* t = localtime(&now);
            t->tm_hour = 0;
            t->tm_min = 0;
            t->tm_sec = 0;
            char today_str[20];
            strftime(today_str, sizeof(today_str), "%Y-%m-%d", t);

            char chat_sql[1024];
            sprintf(chat_sql, "SELECT COUNT(*) FROM chat_message WHERE (from_id='%s' OR to_id='%s') AND DATE(create_time) = '%s'",
                escaped_uid, escaped_uid, today_str);
            if (mysql_query(conn, chat_sql) == 0) {
                MYSQL_RES* res = mysql_store_result(conn);
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row) chat_count = atoi(row[0]);
                mysql_free_result(res);
            }

            resp_json["success"] = true;
            resp_json["data"] = {
                {"friends", friend_count},
                {"matches", match_count},
                {"chats", chat_count}
            };
        }
    }
    // 默认接口
    else {
        resp_json["success"] = true;
        resp_json["message"] = "请求成功";
    }

    // 发送响应
    string resp = resp_headers + resp_json.dump();
    send(client_fd, resp.c_str(), resp.size(), 0);
    // 线程结束前关闭连接
    mysql_close(conn);
    close(client_fd);
    pthread_exit(NULL);
}

// 主函数
int main() {
    // 初始化随机数种子
    srand(time(0));
    // 初始化MySQL库
    if (!initMySQL()) return -1;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);

    cout << "? 服务器启动成功！端口：" << PORT << endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int* client_fd = new int;
        *client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        pthread_t tid;
        pthread_create(&tid, NULL, handleClient, (void*)client_fd);
        pthread_detach(tid);
    }

    // 程序退出时关闭MySQL库
    mysql_library_end();
    close(server_fd);
    return 0;
}
