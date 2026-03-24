#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <map>

// 【关键】包含公共头文件和数据库头文件
#include "common.h"
#include "db.h"

#define SERV_PORT 9527
#define MAXLINE 8192

// 全局变量（继续使用内存缓存，减少数据库查询）
User g_users[MAX_USER];
int g_user_cnt = 0;
pthread_mutex_t g_mutex;

std::map<std::string, int> g_pending_mode;

// 错误处理函数
void sys_err(const char* str) {
    perror(str);
    exit(1);
}

// ===================== 核心查找函数（已重写为内存+数据库混合） =====================
User* find_user(const char* id) {
    pthread_mutex_lock(&g_mutex);

    // 1. 先在内存缓存中找
    for (int i = 0; i < g_user_cnt; i++) {
        if (strcmp(g_users[i].id, id) == 0) {
            pthread_mutex_unlock(&g_mutex);
            return &g_users[i];
        }
    }

    // 2. 内存中没有，从数据库加载
    if (g_user_cnt >= MAX_USER) {
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }

    User new_user;
    if (db_load_user(id, new_user)) {
        // 数据库找到了，加载到内存
        g_users[g_user_cnt] = new_user;
        // 同时加载好友和申请
        db_load_friends(id, g_users[g_user_cnt].friends, g_users[g_user_cnt].friend_cnt);
        db_load_apps(id, g_users[g_user_cnt].apps, g_users[g_user_cnt].app_cnt);
        User* ret = &g_users[g_user_cnt++];
        pthread_mutex_unlock(&g_mutex);
        return ret;
    }
    else {
        // 数据库也没有，创建新用户
        strcpy(g_users[g_user_cnt].id, id);
        g_users[g_user_cnt].current_mode = MODE_NONE;
        g_users[g_user_cnt].friend_cnt = 0;
        g_users[g_user_cnt].app_cnt = 0;
        User* ret = &g_users[g_user_cnt++];
        db_save_user(*ret); // 保存到数据库
        pthread_mutex_unlock(&g_mutex);
        return ret;
    }
}


// 辅助函数：检查指定模式的核心信息是否已填写
bool is_mode_info_filled(User* user, int mode) {
    switch (mode) {
    case MODE_STUDY:
        return (strlen(user->study_subject) > 0 && strlen(user->study_grade) > 0);
    case MODE_POSTGRAD:
        return (strlen(user->postgrad_major) > 0 && strlen(user->postgrad_school) > 0);
    case MODE_FRIEND:
        return (strlen(user->friend_hobby) > 0 && strlen(user->friend_personality) > 0);
    case MODE_LOVE:
        return (strlen(user->love_gender) > 0 && strlen(user->love_intro) > 0);
    default:
        return true;
    }
}

// 辅助函数：获取模式名称
const char* get_mode_name(int mode) {
    static const char* names[] = { "未选择", "学习搭子", "考研搭子", "兴趣交友", "异性恋爱" };
    return names[mode];
}


void handle_client(int cfd) {
    char buf[MAXLINE];
    int n;

    // 1. 接收学号（登录）
    n = read(cfd, buf, sizeof(buf) - 1);
    if (n <= 0) { close(cfd); return; }
    buf[n] = '\0';
    char my_id[20];
    strncpy(my_id, buf, sizeof(my_id));

    // 查找/创建用户
    User* me = find_user(my_id);
    if (!me) {
        const char* err = "❌ 服务器已满，请稍后再试！\n";
        write(cfd, err, strlen(err));
        close(cfd);
        return;
    }

    // 2. 发送登录回包（当前模式 + 未读申请）
    char login_resp[MAXLINE] = { 0 };
    snprintf(login_resp, sizeof(login_resp), "MODE:%d\nPENDING_CNT:%d\n", me->current_mode, me->app_cnt);
    for (int i = 0; i < me->app_cnt; i++) {
        char app_line[200];
        snprintf(app_line, sizeof(app_line), "APP:%s|%s\n", me->apps[i].from_id, me->apps[i].content);
        strncat(login_resp, app_line, sizeof(login_resp) - strlen(login_resp) - 1);
    }
    write(cfd, login_resp, strlen(login_resp));

    // 3. 循环处理客户端指令
    while (1) {
        memset(buf, 0, sizeof(buf));
        n = read(cfd, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';

        // 过滤空指令/换行，避免无效的未知指令
        if (strlen(buf) == 0 || strcmp(buf, "\n") == 0 || strcmp(buf, "\r\n") == 0) {
            continue;
        }
        // 去掉末尾的换行符
        char* p = strchr(buf, '\n');
        if (p) *p = '\0';
        p = strchr(buf, '\r');
        if (p) *p = '\0';

        pthread_mutex_lock(&g_mutex);
        char resp[MAXLINE] = { 0 };
        bool cmd_handled = false;

        // --- 按指令长度从长到短判断，避免短指令提前匹配导致漏判 ---
        // 1. 好友申请/聊天指令（@开头）
        if (strncmp(buf, "@", 1) == 0) {
            char* colon1 = strchr(buf, ':');
            if (colon1) {
                *colon1 = '\0';
                char to_id[20];
                strncpy(to_id, buf + 1, sizeof(to_id));
                char* content = colon1 + 1;

                if (strncmp(content, "REQUEST:", 8) == 0) {
                    char* req_content = content + 8;
                    FriendApp app;
                    strncpy(app.from_id, me->id, sizeof(app.from_id));
                    strncpy(app.to_id, to_id, sizeof(app.to_id));
                    strncpy(app.content, req_content, sizeof(app.content));
                    app.status = 0;
                    db_save_app(app);
                    snprintf(resp, sizeof(resp), "✅ 申请已发送！\n");
                    cmd_handled = true;
                }
                else {
                    snprintf(resp, sizeof(resp), "📨 消息已发送！\n");
                    cmd_handled = true;
                }
            }
        }
        // 2. 更新信息指令
        else if (strncmp(buf, "UPDATE:", 7) == 0) {
            char* data = buf + 7;
            int mode = atoi(data);
            char* content = strchr(data, ':');
            if (content) {
                content++;
                char* pipe = strchr(content, '|');
                if (pipe) *pipe = '\0';

                if (mode == 1) {
                    strncpy(me->study_subject, content, sizeof(me->study_subject));
                    if (pipe) strncpy(me->study_grade, pipe + 1, sizeof(me->study_grade));
                }
                else if (mode == 2) {
                    strncpy(me->postgrad_major, content, sizeof(me->postgrad_major));
                    if (pipe) strncpy(me->postgrad_school, pipe + 1, sizeof(me->postgrad_school));
                }
                else if (mode == 3) {
                    strncpy(me->friend_hobby, content, sizeof(me->friend_hobby));
                    if (pipe) strncpy(me->friend_personality, pipe + 1, sizeof(me->friend_personality));
                }
                else if (mode == 4) {
                    strncpy(me->love_gender, content, sizeof(me->love_gender));
                    if (pipe) strncpy(me->love_intro, pipe + 1, sizeof(me->love_intro));
                }
                db_save_user(*me);
                snprintf(resp, sizeof(resp), "✅ 信息更新成功！\n");
                cmd_handled = true;
            }
        }
        // 3. 模式切换/选择指令（统一处理 MODE: 和 SWITCH:）
        else if (strncmp(buf, "SWITCH:", 7) == 0 || strncmp(buf, "MODE:", 5) == 0) {
            int target_mode = 0;
            if (strncmp(buf, "MODE:", 5) == 0) {
                target_mode = atoi(buf + 5);
            }
            else {
                target_mode = atoi(buf + 7);
            }

            // 检查目标模式的信息是否已填写
            if (!is_mode_info_filled(me, target_mode)) {
                // 信息未填写：记住目标模式，返回NEED_FILL让客户端跳转填写
                g_pending_mode[me->id] = target_mode;
                snprintf(resp, sizeof(resp), "NEED_FILL:%d\n", target_mode);
                cmd_handled = true;
            }
            else {
                // 信息已填写：直接切换模式
                me->current_mode = (MatchMode)target_mode;
                snprintf(resp, sizeof(resp), "✅ 已切换到【%s】模式！\n", get_mode_name(target_mode));
                db_save_user(*me);
                cmd_handled = true;
            }
        }
        // 4. 学习信息保存指令
        else if (strncmp(buf, "STUDY:", 6) == 0) {
            char* data = buf + 6;
            char* pipe = strchr(data, '|');
            if (pipe) {
                *pipe = '\0';
                strncpy(me->study_subject, data, sizeof(me->study_subject));
                strncpy(me->study_grade, pipe + 1, sizeof(me->study_grade));

                // 检查是否有待切换的模式
                if (g_pending_mode.find(me->id) != g_pending_mode.end()) {
                    int pending_mode = g_pending_mode[me->id];
                    if (is_mode_info_filled(me, pending_mode)) {
                        // 自动切换模式
                        me->current_mode = (MatchMode)pending_mode;
                        db_save_user(*me);
                        g_pending_mode.erase(me->id);
                        snprintf(resp, sizeof(resp), "✅ 学习信息已保存！已自动切换到【%s】模式！\n", get_mode_name(pending_mode));
                    }
                    else {
                        snprintf(resp, sizeof(resp), "✅ 学习信息已保存！\n");
                        db_save_user(*me);
                    }
                }
                else {
                    snprintf(resp, sizeof(resp), "✅ 学习信息已保存！\n");
                    db_save_user(*me);
                }
                cmd_handled = true;
            }
        }
        // 5. 考研信息保存指令
        else if (strncmp(buf, "POSTGRAD:", 9) == 0) {
            char* data = buf + 9;
            char* pipe = strchr(data, '|');
            if (pipe) {
                *pipe = '\0';
                strncpy(me->postgrad_major, data, sizeof(me->postgrad_major));
                strncpy(me->postgrad_school, pipe + 1, sizeof(me->postgrad_school));

                // 检查是否有待切换的模式
                if (g_pending_mode.find(me->id) != g_pending_mode.end()) {
                    int pending_mode = g_pending_mode[me->id];
                    if (is_mode_info_filled(me, pending_mode)) {
                        me->current_mode = (MatchMode)pending_mode;
                        db_save_user(*me);
                        g_pending_mode.erase(me->id);
                        snprintf(resp, sizeof(resp), "✅ 考研信息已保存！已自动切换到【%s】模式！\n", get_mode_name(pending_mode));
                    }
                    else {
                        snprintf(resp, sizeof(resp), "✅ 考研信息已保存！\n");
                        db_save_user(*me);
                    }
                }
                else {
                    snprintf(resp, sizeof(resp), "✅ 考研信息已保存！\n");
                    db_save_user(*me);
                }
                cmd_handled = true;
            }
        }
        // 6. 交友信息保存指令
        else if (strncmp(buf, "FRIEND:", 7) == 0) {
            char* data = buf + 7;
            char* pipe = strchr(data, '|');
            if (pipe) {
                *pipe = '\0';
                strncpy(me->friend_hobby, data, sizeof(me->friend_hobby));
                strncpy(me->friend_personality, pipe + 1, sizeof(me->friend_personality));

                // 检查是否有待切换的模式
                if (g_pending_mode.find(me->id) != g_pending_mode.end()) {
                    int pending_mode = g_pending_mode[me->id];
                    if (is_mode_info_filled(me, pending_mode)) {
                        me->current_mode = (MatchMode)pending_mode;
                        db_save_user(*me);
                        g_pending_mode.erase(me->id);
                        snprintf(resp, sizeof(resp), "✅ 交友信息已保存！已自动切换到【%s】模式！\n", get_mode_name(pending_mode));
                    }
                    else {
                        snprintf(resp, sizeof(resp), "✅ 交友信息已保存！\n");
                        db_save_user(*me);
                    }
                }
                else {
                    snprintf(resp, sizeof(resp), "✅ 交友信息已保存！\n");
                    db_save_user(*me);
                }
                cmd_handled = true;
            }
        }
        // 7. 恋爱信息保存指令
        else if (strncmp(buf, "LOVE:", 5) == 0) {
            char* data = buf + 5;
            char* pipe = strchr(data, '|');
            if (pipe) {
                *pipe = '\0';
                strncpy(me->love_gender, data, sizeof(me->love_gender));
                strncpy(me->love_intro, pipe + 1, sizeof(me->love_intro));

                // 检查是否有待切换的模式
                if (g_pending_mode.find(me->id) != g_pending_mode.end()) {
                    int pending_mode = g_pending_mode[me->id];
                    if (is_mode_info_filled(me, pending_mode)) {
                        me->current_mode = (MatchMode)pending_mode;
                        db_save_user(*me);
                        g_pending_mode.erase(me->id);
                        snprintf(resp, sizeof(resp), "✅ 恋爱信息已保存！已自动切换到【%s】模式！\n", get_mode_name(pending_mode));
                    }
                    else {
                        snprintf(resp, sizeof(resp), "✅ 恋爱信息已保存！\n");
                        db_save_user(*me);
                    }
                }
                else {
                    snprintf(resp, sizeof(resp), "✅ 恋爱信息已保存！\n");
                    db_save_user(*me);
                }
                cmd_handled = true;
            }
        }
        // 8. 期望信息保存指令
        else if (strncmp(buf, "EXP:", 4) == 0) {
            char* data = buf + 4;
            if (me->current_mode == MODE_STUDY) {
                char* pipe = strchr(data, '|');
                if (pipe) {
                    *pipe = '\0';
                    strncpy(me->exp_study_subject, data, sizeof(me->exp_study_subject));
                    strncpy(me->exp_study_grade, pipe + 1, sizeof(me->exp_study_grade));
                }
            }
            else if (me->current_mode == MODE_POSTGRAD) {
                char* pipe = strchr(data, '|');
                if (pipe) {
                    *pipe = '\0';
                    strncpy(me->exp_postgrad_major, data, sizeof(me->exp_postgrad_major));
                    strncpy(me->exp_postgrad_school, pipe + 1, sizeof(me->exp_postgrad_school));
                }
            }
            else if (me->current_mode == MODE_FRIEND) {
                char* pipe = strchr(data, '|');
                if (pipe) {
                    *pipe = '\0';
                    strncpy(me->exp_friend_hobby, data, sizeof(me->exp_friend_hobby));
                    strncpy(me->exp_friend_personality, pipe + 1, sizeof(me->exp_friend_personality));
                }
            }
            else if (me->current_mode == MODE_LOVE) {
                strncpy(me->exp_love_gender, data, sizeof(me->exp_love_gender));
            }
            snprintf(resp, sizeof(resp), "✅ 期望信息已保存！\n");
            db_save_user(*me);
            cmd_handled = true;
        }
        // 9. 同意申请指令
        else if (strncmp(buf, "AGREE:", 6) == 0) {
            char* from_id = buf + 6;
            db_update_app_status(from_id, me->id, 1);
            db_add_friend(me->id, from_id);
            db_load_friends(me->id, me->friends, me->friend_cnt);
            snprintf(resp, sizeof(resp), "✅ 已同意申请，你们已经是好友了！\n");
            cmd_handled = true;
        }
        // 10. 拒绝申请指令
        else if (strncmp(buf, "REFUSE:", 7) == 0) {
            char* from_id = buf + 7;
            db_update_app_status(from_id, me->id, 2);
            snprintf(resp, sizeof(resp), "✅ 已拒绝申请！\n");
            cmd_handled = true;
        }
        // 11. 查询当前模式指令
        else if (strcmp(buf, "QUERY_MODE") == 0 || strcmp(buf, "MODE") == 0) {
            snprintf(resp, sizeof(resp), "📌 当前模式：%s\n", get_mode_name(me->current_mode));
            cmd_handled = true;
        }
        // 12. 查询所有信息指令
        else if (strcmp(buf, "QUERY_ALL_INFO") == 0 || strcmp(buf, "INFO") == 0) {
            snprintf(resp, sizeof(resp),
                "📌 你的信息：\n学号：%s\n当前模式：%s\n"
                "学习：%s | %s\n考研：%s | %s\n交友：%s | %s\n恋爱：%s | %.30s\n",
                me->id, get_mode_name(me->current_mode),
                me->study_subject, me->study_grade,
                me->postgrad_major, me->postgrad_school,
                me->friend_hobby, me->friend_personality,
                me->love_gender, me->love_intro);
            cmd_handled = true;
        }
        // 13. 匹配指令
        else if (strcmp(buf, "MATCH") == 0) {
            snprintf(resp, sizeof(resp), "🔍 正在为你匹配...\n（匹配算法可自行扩展）\n");
            cmd_handled = true;
        }
        // 14. 退出指令
        else if (strcmp(buf, "quit") == 0) {
            snprintf(resp, sizeof(resp), "👋 再见！\n");
            cmd_handled = true;
        }

        // 只有真的没匹配到指令，才返回未知指令
        if (!cmd_handled) {
            snprintf(resp, sizeof(resp), "❓ 未知指令：%s\n", buf);
        }

        pthread_mutex_unlock(&g_mutex);

        // 发送回复给客户端
        if (strlen(resp) > 0) {
            write(cfd, resp, strlen(resp));
        }

        // 退出指令，关闭连接
        if (strcmp(buf, "quit") == 0) break;
    }

    close(cfd);
}




// ===================== 线程函数 =====================
void* thread_main(void* arg) {
    int cfd = *(int*)arg;
    free(arg);
    pthread_detach(pthread_self());
    handle_client(cfd);
    return NULL;
}

// ===================== 主函数 =====================
int main() {
    int lfd, cfd;
    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;
    pthread_t tid;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) sys_err("socket创建失败");

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        sys_err("bind失败");

    if (listen(lfd, 128) == -1) sys_err("listen失败");

    // ========== 【关键修改】初始化数据库 ==========
    if (!init_mysql()) {
        return 1;
    }

    // ========== 【关键修改】从数据库加载所有用户到内存 ==========
    pthread_mutex_init(&g_mutex, NULL);
    db_load_all_users(g_users, g_user_cnt);
    // 为每个用户加载好友和申请
    for (int i = 0; i < g_user_cnt; i++) {
        db_load_friends(g_users[i].id, g_users[i].friends, g_users[i].friend_cnt);
        db_load_apps(g_users[i].id, g_users[i].apps, g_users[i].app_cnt);
    }

    std::cout << "✅ 服务器启动成功 端口：" << SERV_PORT << std::endl;

    while (1) {
        clit_addr_len = sizeof(clit_addr);
        cfd = accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
        if (cfd == -1) sys_err("accept失败");

        int* p = (int*)malloc(sizeof(int));
        *p = cfd;
        pthread_create(&tid, NULL, thread_main, p);
    }

    // ========== 【关键修改】程序结束时关闭数据库 ==========
    close_mysql();
    close(lfd);
    pthread_mutex_destroy(&g_mutex);
    return 0;
}