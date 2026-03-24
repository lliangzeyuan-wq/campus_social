#include "db.h"
#include <cstdio>
#include <cstring>

// 定义全局 MySQL 连接句柄
MYSQL* g_mysql_conn = nullptr;

// ===================== 核心连接函数 =====================
bool init_mysql() {
    // 1. 初始化 MySQL 对象
    g_mysql_conn = mysql_init(nullptr);
    if (!g_mysql_conn) {
        fprintf(stderr, "[MySQL] 初始化失败！\n");
        return false;
    }

    // 2. 设置 UTF-8 字符集，支持中文和 emoji
    mysql_options(g_mysql_conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    // 3. 连接数据库
    if (!mysql_real_connect(g_mysql_conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, nullptr, 0)) {
        fprintf(stderr, "[MySQL] 连接失败：%s\n", mysql_error(g_mysql_conn));
        mysql_close(g_mysql_conn);
        g_mysql_conn = nullptr;
        return false;
    }

    printf("[MySQL] 连接成功！\n");
    return true;
}

void close_mysql() {
    if (g_mysql_conn) {
        mysql_close(g_mysql_conn);
        g_mysql_conn = nullptr;
        printf("[MySQL] 已断开连接\n");
    }
}

// ===================== 用户操作 =====================
bool db_save_user(const User& user) {
    char sql[2048] = { 0 };
    // 插入或更新用户数据（覆盖所有字段）
    snprintf(sql, sizeof(sql),
        "INSERT INTO user ("
        "id, current_mode, "
        "study_subject, study_grade, postgrad_major, postgrad_school, friend_hobby, friend_personality, love_gender, love_intro, "
        "exp_study_subject, exp_study_grade, exp_postgrad_major, exp_postgrad_school, exp_friend_hobby, exp_friend_personality, exp_love_gender"
        ") VALUES ("
        "'%s', %d, "
        "'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', "
        "'%s', '%s', '%s', '%s', '%s', '%s', '%s'"
        ") ON DUPLICATE KEY UPDATE "
        "current_mode=%d, "
        "study_subject='%s', study_grade='%s', postgrad_major='%s', postgrad_school='%s', friend_hobby='%s', friend_personality='%s', love_gender='%s', love_intro='%s', "
        "exp_study_subject='%s', exp_study_grade='%s', exp_postgrad_major='%s', exp_postgrad_school='%s', exp_friend_hobby='%s', exp_friend_personality='%s', exp_love_gender='%s'",
        user.id, user.current_mode,
        user.study_subject, user.study_grade, user.postgrad_major, user.postgrad_school, user.friend_hobby, user.friend_personality, user.love_gender, user.love_intro,
        user.exp_study_subject, user.exp_study_grade, user.exp_postgrad_major, user.exp_postgrad_school, user.exp_friend_hobby, user.exp_friend_personality, user.exp_love_gender,
        user.current_mode,
        user.study_subject, user.study_grade, user.postgrad_major, user.postgrad_school, user.friend_hobby, user.friend_personality, user.love_gender, user.love_intro,
        user.exp_study_subject, user.exp_study_grade, user.exp_postgrad_major, user.exp_postgrad_school, user.exp_friend_hobby, user.exp_friend_personality, user.exp_love_gender);

    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 保存用户失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }
    return true;
}

bool db_load_user(const char* id, User& user) {
    char sql[512] = { 0 };
    snprintf(sql, sizeof(sql),
        "SELECT id, current_mode, "
        "study_subject, study_grade, postgrad_major, postgrad_school, friend_hobby, friend_personality, love_gender, love_intro, "
        "exp_study_subject, exp_study_grade, exp_postgrad_major, exp_postgrad_school, exp_friend_hobby, exp_friend_personality, exp_love_gender "
        "FROM user WHERE id='%s' AND is_deleted=0",
        id);

    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 查询用户失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }

    MYSQL_RES* result = mysql_store_result(g_mysql_conn);
    if (!result) return false;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }

    // 赋值给 User 结构体
    strncpy(user.id, row[0] ? row[0] : "", sizeof(user.id));
    user.current_mode = (MatchMode)(row[1] ? atoi(row[1]) : 0);

    // 自身信息
    strncpy(user.study_subject, row[2] ? row[2] : "", sizeof(user.study_subject));
    strncpy(user.study_grade, row[3] ? row[3] : "", sizeof(user.study_grade));
    strncpy(user.postgrad_major, row[4] ? row[4] : "", sizeof(user.postgrad_major));
    strncpy(user.postgrad_school, row[5] ? row[5] : "", sizeof(user.postgrad_school));
    strncpy(user.friend_hobby, row[6] ? row[6] : "", sizeof(user.friend_hobby));
    strncpy(user.friend_personality, row[7] ? row[7] : "", sizeof(user.friend_personality));
    strncpy(user.love_gender, row[8] ? row[8] : "", sizeof(user.love_gender));
    strncpy(user.love_intro, row[9] ? row[9] : "", sizeof(user.love_intro));

    // 期望信息
    strncpy(user.exp_study_subject, row[10] ? row[10] : "", sizeof(user.exp_study_subject));
    strncpy(user.exp_study_grade, row[11] ? row[11] : "", sizeof(user.exp_study_grade));
    strncpy(user.exp_postgrad_major, row[12] ? row[12] : "", sizeof(user.exp_postgrad_major));
    strncpy(user.exp_postgrad_school, row[13] ? row[13] : "", sizeof(user.exp_postgrad_school));
    strncpy(user.exp_friend_hobby, row[14] ? row[14] : "", sizeof(user.exp_friend_hobby));
    strncpy(user.exp_friend_personality, row[15] ? row[15] : "", sizeof(user.exp_friend_personality));
    strncpy(user.exp_love_gender, row[16] ? row[16] : "", sizeof(user.exp_love_gender));

    // 初始化好友和申请（后续单独加载）
    user.friend_cnt = 0;
    user.app_cnt = 0;

    mysql_free_result(result);
    return true;
}

bool db_delete_user(const char* id) {
    char sql[256] = { 0 };
    snprintf(sql, sizeof(sql), "UPDATE user SET is_deleted=1 WHERE id='%s'", id);
    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 删除用户失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }
    return true;
}

bool db_load_all_users(User users[], int& user_cnt) {
    char sql[] = "SELECT id, current_mode, "
        "study_subject, study_grade, postgrad_major, postgrad_school, friend_hobby, friend_personality, love_gender, love_intro, "
        "exp_study_subject, exp_study_grade, exp_postgrad_major, exp_postgrad_school, exp_friend_hobby, exp_friend_personality, exp_love_gender "
        "FROM user WHERE is_deleted=0";

    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 查询所有用户失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }

    MYSQL_RES* result = mysql_store_result(g_mysql_conn);
    if (!result) return false;

    user_cnt = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && user_cnt < MAX_USER) {
        User& user = users[user_cnt++];
        strncpy(user.id, row[0] ? row[0] : "", sizeof(user.id));
        user.current_mode = (MatchMode)(row[1] ? atoi(row[1]) : 0);

        strncpy(user.study_subject, row[2] ? row[2] : "", sizeof(user.study_subject));
        strncpy(user.study_grade, row[3] ? row[3] : "", sizeof(user.study_grade));
        strncpy(user.postgrad_major, row[4] ? row[4] : "", sizeof(user.postgrad_major));
        strncpy(user.postgrad_school, row[5] ? row[5] : "", sizeof(user.postgrad_school));
        strncpy(user.friend_hobby, row[6] ? row[6] : "", sizeof(user.friend_hobby));
        strncpy(user.friend_personality, row[7] ? row[7] : "", sizeof(user.friend_personality));
        strncpy(user.love_gender, row[8] ? row[8] : "", sizeof(user.love_gender));
        strncpy(user.love_intro, row[9] ? row[9] : "", sizeof(user.love_intro));

        strncpy(user.exp_study_subject, row[10] ? row[10] : "", sizeof(user.exp_study_subject));
        strncpy(user.exp_study_grade, row[11] ? row[11] : "", sizeof(user.exp_study_grade));
        strncpy(user.exp_postgrad_major, row[12] ? row[12] : "", sizeof(user.exp_postgrad_major));
        strncpy(user.exp_postgrad_school, row[13] ? row[13] : "", sizeof(user.exp_postgrad_school));
        strncpy(user.exp_friend_hobby, row[14] ? row[14] : "", sizeof(user.exp_friend_hobby));
        strncpy(user.exp_friend_personality, row[15] ? row[15] : "", sizeof(user.exp_friend_personality));
        strncpy(user.exp_love_gender, row[16] ? row[16] : "", sizeof(user.exp_love_gender));

        user.friend_cnt = 0;
        user.app_cnt = 0;
    }

    mysql_free_result(result);
    return true;
}

// ===================== 好友申请操作 =====================
bool db_save_app(const FriendApp& app) {
    char sql[512] = { 0 };
    snprintf(sql, sizeof(sql),
        "INSERT INTO invitation (from_id, to_id, content, status) "
        "VALUES ('%s', '%s', '%s', %d) "
        "ON DUPLICATE KEY UPDATE content='%s', status=%d",
        app.from_id, app.to_id, app.content, app.status,
        app.content, app.status);

    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 保存申请失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }
    return true;
}

bool db_load_apps(const char* to_id, FriendApp apps[], int& app_cnt) {
    char sql[512] = { 0 };
    snprintf(sql, sizeof(sql),
        "SELECT from_id, to_id, content, status FROM invitation WHERE to_id='%s' ORDER BY create_time DESC",
        to_id);

    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 查询申请失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }

    MYSQL_RES* result = mysql_store_result(g_mysql_conn);
    if (!result) return false;

    app_cnt = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && app_cnt < MAX_APPS) {
        FriendApp& app = apps[app_cnt++];
        strncpy(app.from_id, row[0] ? row[0] : "", sizeof(app.from_id));
        strncpy(app.to_id, row[1] ? row[1] : "", sizeof(app.to_id));
        strncpy(app.content, row[2] ? row[2] : "", sizeof(app.content));
        app.status = atoi(row[3]);
    }

    mysql_free_result(result);
    return true;
}

bool db_update_app_status(const char* from_id, const char* to_id, int status) {
    char sql[512] = { 0 };
    snprintf(sql, sizeof(sql),
        "UPDATE invitation SET status=%d WHERE from_id='%s' AND to_id='%s' AND status=0",
        status, from_id, to_id);

    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 更新申请状态失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }
    return true;
}

// ===================== 好友关系操作 =====================
bool db_add_friend(const char* user_id_1, const char* user_id_2) {
    char sql[512] = { 0 };
    snprintf(sql, sizeof(sql),
        "INSERT IGNORE INTO friend_relation (user_id_1, user_id_2) VALUES ('%s', '%s')",
        user_id_1, user_id_2);

    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 添加好友失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }
    return true;
}

bool db_load_friends(const char* user_id, char friends[][20], int& friend_cnt) {
    char sql[512] = { 0 };
    // 双向查询：用户作为 user_id_1 或 user_id_2 的情况
    snprintf(sql, sizeof(sql),
        "SELECT user_id_2 FROM friend_relation WHERE user_id_1='%s' "
        "UNION "
        "SELECT user_id_1 FROM friend_relation WHERE user_id_2='%s'",
        user_id, user_id);

    if (mysql_query(g_mysql_conn, sql) != 0) {
        fprintf(stderr, "[MySQL] 查询好友失败：%s\n", mysql_error(g_mysql_conn));
        return false;
    }

    MYSQL_RES* result = mysql_store_result(g_mysql_conn);
    if (!result) return false;

    friend_cnt = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && friend_cnt < MAX_USER) {
        strncpy(friends[friend_cnt++], row[0] ? row[0] : "", 20);
    }

    mysql_free_result(result);
    return true;
}