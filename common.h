#ifndef COMMON_H
#define COMMON_H

#define SERV_PORT 9527
#define MAX_USER 100
#define USER_FILE "users.dat"
#define MAX_APPS 50

// 匹配模式枚举
typedef enum {
    MODE_NONE = 0,
    MODE_STUDY = 1,
    MODE_POSTGRAD = 2,
    MODE_FRIEND = 3,
    MODE_LOVE = 4
} MatchMode;

// 好友申请结构体
typedef struct {
    char from_id[20];
    char to_id[20];
    char content[100];
    int status; // 0:待处理, 1:已同意, 2:已拒绝
} FriendApp;

// 用户核心结构体（完全照搬 server.cpp 里的定义）
typedef struct {
    char id[20];
    MatchMode current_mode;

    // --- 自身信息 ---
    char study_subject[50], study_grade[20];
    char postgrad_major[50], postgrad_school[50];
    char friend_hobby[50], friend_personality[20];
    char love_gender[10], love_intro[100];

    // --- 期望信息 ---
    char exp_study_subject[50], exp_study_grade[20];
    char exp_postgrad_major[50], exp_postgrad_school[50];
    char exp_friend_hobby[50], exp_friend_personality[20];
    char exp_love_gender[10];

    // --- 好友关系 ---
    char friends[MAX_USER][20];
    int friend_cnt;

    // --- 好友申请 ---
    FriendApp apps[MAX_APPS];
    int app_cnt;
} User;

#endif // COMMON_H