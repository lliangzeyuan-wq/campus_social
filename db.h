#ifndef DB_H
#define DB_H

// MySQL 头文件
#include <mysql/mysql.h>
// C++ 标准库
#include <string>
// 引入你的项目结构体（User 和 FriendApp）
#include "common.h"

// ===================== 数据库配置（你的专属配置） =====================
#define DB_HOST     "localhost"
#define DB_PORT     3306
#define DB_USER     "campus"
#define DB_PASS     "Ll19299637611@"
#define DB_NAME     "partner"

// ===================== 全局连接句柄 =====================
extern MYSQL* g_mysql_conn;

// ===================== 核心函数声明 =====================
/**
 * @brief 初始化 MySQL 连接
 * @return 成功返回 true，失败返回 false
 */
bool init_mysql();

/**
 * @brief 关闭 MySQL 连接
 */
void close_mysql();

// ===================== 用户操作 =====================
/**
 * @brief 保存用户到数据库（插入或更新）
 * @param user 用户结构体
 * @return 成功返回 true
 */
bool db_save_user(const User& user);

/**
 * @brief 从数据库加载单个用户
 * @param id 学号
 * @param user 输出参数，存储查询到的用户信息
 * @return 找到返回 true
 */
bool db_load_user(const char* id, User& user);

/**
 * @brief 软删除用户（设置 is_deleted=1）
 * @param id 学号
 * @return 成功返回 true
 */
bool db_delete_user(const char* id);

/**
 * @brief 从数据库加载所有用户（用于 server.cpp 启动和 manage_users.cpp）
 * @param users 输出数组
 * @param user_cnt 输出用户数量
 * @return 成功返回 true
 */
bool db_load_all_users(User users[], int& user_cnt);

// ===================== 好友申请操作 =====================
/**
 * @brief 保存好友申请（插入或更新）
 * @param app 申请结构体
 * @return 成功返回 true
 */
bool db_save_app(const FriendApp& app);

/**
 * @brief 加载指定用户的所有好友申请
 * @param to_id 接收方学号
 * @param apps 输出数组
 * @param app_cnt 输出申请数量
 * @return 成功返回 true
 */
bool db_load_apps(const char* to_id, FriendApp apps[], int& app_cnt);

/**
 * @brief 更新好友申请状态
 * @param from_id 发起方学号
 * @param to_id 接收方学号
 * @param status 新状态（1-同意 2-拒绝）
 * @return 成功返回 true
 */
bool db_update_app_status(const char* from_id, const char* to_id, int status);

// ===================== 好友关系操作 =====================
/**
 * @brief 添加好友关系
 * @param user_id_1 用户1学号
 * @param user_id_2 用户2学号
 * @return 成功返回 true
 */
bool db_add_friend(const char* user_id_1, const char* user_id_2);

/**
 * @brief 加载指定用户的所有好友
 * @param user_id 用户学号
 * @param friends 输出好友学号数组
 * @param friend_cnt 输出好友数量
 * @return 成功返回 true
 */
bool db_load_friends(const char* user_id, char friends[][20], int& friend_cnt);

#endif // DB_H