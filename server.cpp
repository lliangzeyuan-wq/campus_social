#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include"common.h"

User g_users[MAX_USER];
int g_user_cnt = 0;
int g_client_fds[MAX_USER];
char g_client_ids[MAX_USER][20];
int g_fd_cnt = 0;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void sys_err(const char* str) { perror(str); exit(1); }
void load_users();
void save_users();
User* find_user(const char* id);
int get_fd_by_id(const char* id);
void do_match(int cfd, const char* id);
void send_app(const char* from, const char* to, const char* content);
void deal_app(const char* to_id, const char* from_id, int opt);
void send_msg(const char* from, const char* to, const char* msg);
void handle_single_cmd(int cfd, User* me, const char* id, char* cmd);

void load_users() {
    FILE* fp = fopen(USER_FILE, "rb");
    if (!fp) { g_user_cnt = 0; return; }
    g_user_cnt = fread(g_users, sizeof(User), MAX_USER, fp);
    fclose(fp);
}

void save_users() {
    FILE* fp = fopen(USER_FILE, "wb");
    if (!fp) return;
    fwrite(g_users, sizeof(User), g_user_cnt, fp);
    fclose(fp);
}

int get_fd_by_id(const char* id) {
    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < g_fd_cnt; i++) {
        if (strcmp(g_client_ids[i], id) == 0) {
            int fd = g_client_fds[i];
            pthread_mutex_unlock(&g_mutex);
            return fd;
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return -1;
}

User* find_user(const char* id) {
    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < g_user_cnt; i++) {
        if (strcmp(g_users[i].id, id) == 0) {
            pthread_mutex_unlock(&g_mutex);
            return &g_users[i];
        }
    }
    if (g_user_cnt >= MAX_USER) {
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }
    strcpy(g_users[g_user_cnt].id, id);
    g_users[g_user_cnt].current_mode = MODE_NONE;
    g_users[g_user_cnt].friend_cnt = 0;
    g_users[g_user_cnt].app_cnt = 0;
    User* ret = &g_users[g_user_cnt++];
    pthread_mutex_unlock(&g_mutex);
    save_users();
    return ret;
}

void do_match(int cfd, const char* id) {
    char res[2048] = { 0 };
    int cnt = 0;

    pthread_mutex_lock(&g_mutex);
    User* me = NULL;
    for (int i = 0; i < g_user_cnt; i++) {
        if (strcmp(g_users[i].id, id) == 0) {
            me = &g_users[i];
            break;
        }
    }
    if (!me) {
        pthread_mutex_unlock(&g_mutex);
        write(cfd, "用户不存在！", strlen("用户不存在！"));
        return;
    }

    for (int i = 0; i < g_user_cnt; i++) {
        User* u = &g_users[i];
        if (strcmp(u->id, id) == 0 || u->current_mode != me->current_mode) continue;

        int ok = 0;
        switch (me->current_mode) {
        case MODE_STUDY:
            ok = (strcmp(u->study_subject, me->exp_study_subject) == 0 && strcmp(u->study_grade, me->exp_study_grade) == 0);
            break;
        case MODE_POSTGRAD:
            ok = (strcmp(u->postgrad_major, me->exp_postgrad_major) == 0 && strcmp(u->postgrad_school, me->exp_postgrad_school) == 0);
            break;
        case MODE_FRIEND:
            ok = (strcmp(u->friend_hobby, me->exp_friend_hobby) == 0 && strcmp(u->friend_personality, me->exp_friend_personality) == 0);
            break;
        case MODE_LOVE:
            ok = (strcmp(u->love_gender, me->exp_love_gender) == 0);
            break;
        default: break;
        }

        if (ok) {
            char tmp[512] = { 0 };
            switch (me->current_mode) {
            case MODE_STUDY:
                snprintf(tmp, sizeof(tmp), "【匹配用户】%s\n  学习科目：%s\n  年级/专业：%s\n\n", u->id, u->study_subject, u->study_grade);
                break;
            case MODE_POSTGRAD:
                snprintf(tmp, sizeof(tmp), "【匹配用户】%s\n  考研专业：%s\n  目标院校：%s\n\n", u->id, u->postgrad_major, u->postgrad_school);
                break;
            case MODE_FRIEND:
                snprintf(tmp, sizeof(tmp), "【匹配用户】%s\n  兴趣爱好：%s\n  性格特点：%s\n\n", u->id, u->friend_hobby, u->friend_personality);
                break;
            case MODE_LOVE:
                snprintf(tmp, sizeof(tmp), "【匹配用户】%s\n  性别：%s\n  个人简介：%s\n\n", u->id, u->love_gender, u->love_intro);
                break;
            default: break;
            }
            strcat(res, tmp);
            cnt++;
        }
    }
    pthread_mutex_unlock(&g_mutex);

    if (cnt == 0) strcpy(res, "未匹配到符合条件的用户！");
    else {
        char header[64];
        snprintf(header, sizeof(header), "✨ 共匹配到 %d 位用户：\n\n", cnt);
        memmove(res + strlen(header), res, strlen(res) + 1);
        memcpy(res, header, strlen(header));
    }
    write(cfd, res, strlen(res));
}

void send_app(const char* from, const char* to, const char* content) {
    User* u = find_user(to);
    if (!u || u->app_cnt >= MAX_APPS) return;
    FriendApp& app = u->apps[u->app_cnt++];
    strcpy(app.from_id, from);
    strcpy(app.to_id, to);
    strcpy(app.content, content);
    app.status = 0;
    save_users();
    int fd = get_fd_by_id(to);
    if (fd != -1) write(fd, "收到新的好友申请！", strlen("收到新的好友申请！"));
}

void deal_app(const char* to_id, const char* from_id, int opt) {
    User* u = find_user(to_id);
    if (!u) return;
    for (int i = 0; i < u->app_cnt; i++) {
        if (strcmp(u->apps[i].from_id, from_id) == 0 && u->apps[i].status == 0) {
            u->apps[i].status = opt;
            if (opt == 1) {
                if (u->friend_cnt < MAX_USER) strcpy(u->friends[u->friend_cnt++], from_id);
                User* f = find_user(from_id);
                if (f && f->friend_cnt < MAX_USER) strcpy(f->friends[f->friend_cnt++], to_id);
            }
            save_users();
            break;
        }
    }
}

void send_msg(const char* from, const char* to, const char* msg) {
    int fd = get_fd_by_id(to);
    if (fd == -1) return;
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s 说：%s", from, msg);
    write(fd, buf, strlen(buf));
}

void handle_single_cmd(int cfd, User* me, const char* id, char* cmd) {
    if (strstr(cmd, "MODE:") == cmd) {
        int m = atoi(cmd + 5);
        me->current_mode = (MatchMode)m;
        save_users();
        write(cfd, "模式选择成功", strlen("模式选择成功"));
    }
    else if (strstr(cmd, "STUDY:") == cmd) {
        sscanf(cmd, "STUDY:%[^|]|%s", me->study_subject, me->study_grade);
        save_users();
        write(cfd, "学习信息保存成功", strlen("学习信息保存成功"));
    }
    else if (strstr(cmd, "POSTGRAD:") == cmd) {
        sscanf(cmd, "POSTGRAD:%[^|]|%s", me->postgrad_major, me->postgrad_school);
        save_users();
        write(cfd, "考研信息保存成功", strlen("考研信息保存成功"));
    }
    else if (strstr(cmd, "FRIEND:") == cmd) {
        sscanf(cmd, "FRIEND:%[^|]|%s", me->friend_hobby, me->friend_personality);
        save_users();
        write(cfd, "兴趣信息保存成功", strlen("兴趣信息保存成功"));
    }
    else if (strstr(cmd, "LOVE:") == cmd) {
        sscanf(cmd, "LOVE:%[^|]|%s", me->love_gender, me->love_intro);
        save_users();
        write(cfd, "恋爱信息保存成功", strlen("恋爱信息保存成功"));
    }
    else if (strstr(cmd, "EXP:") == cmd) {
        if (me->current_mode == MODE_STUDY)
            sscanf(cmd, "EXP:%[^|]|%s", me->exp_study_subject, me->exp_study_grade);
        else if (me->current_mode == MODE_POSTGRAD)
            sscanf(cmd, "EXP:%[^|]|%s", me->exp_postgrad_major, me->exp_postgrad_school);
        else if (me->current_mode == MODE_FRIEND)
            sscanf(cmd, "EXP:%[^|]|%s", me->exp_friend_hobby, me->exp_friend_personality);
        else if (me->current_mode == MODE_LOVE)
            sscanf(cmd, "EXP:%s", me->exp_love_gender);
        save_users();
        write(cfd, "期望信息保存成功", strlen("期望信息保存成功"));
    }
    else if (strcmp(cmd, "MATCH") == 0) do_match(cfd, id);
    else if (cmd[0] == '@') {
        char to[20], type[20], content[100];
        sscanf(cmd, "@%[^:]:%[^:]:%s", to, type, content);
        if (strcmp(type, "REQUEST") == 0) send_app(id, to, content);
        else send_msg(id, to, content);
        write(cfd, "操作成功", strlen("操作成功"));
    }
    else if (strstr(cmd, "AGREE:") == cmd) {
        char f[20]; sscanf(cmd, "AGREE:%s", f);
        deal_app(id, f, 1);
        write(cfd, "已同意申请", strlen("已同意申请"));
    }
    else if (strstr(cmd, "REFUSE:") == cmd) {
        char f[20]; sscanf(cmd, "REFUSE:%s", f);
        deal_app(id, f, 2);
        write(cfd, "已拒绝申请", strlen("已拒绝申请"));
    }
    else if (strcmp(cmd, "QUERY_MODE") == 0) {
        char res[100];
        const char* mode_str;
        switch (me->current_mode) {
        case MODE_STUDY: mode_str = "学习搭子"; break;
        case MODE_POSTGRAD: mode_str = "考研搭子"; break;
        case MODE_FRIEND: mode_str = "兴趣交友"; break;
        case MODE_LOVE: mode_str = "异性恋爱"; break;
        default: mode_str = "未选择模式"; break;
        }
        snprintf(res, sizeof(res), "当前模式：%s", mode_str);
        write(cfd, res, strlen(res));
    }
    else if (strcmp(cmd, "QUERY_ALL_INFO") == 0) {
        char res[2048] = { 0 };
        int off = 0;
        snprintf(res + off, sizeof(res) - off, "========== 你的所有信息 ==========\n");
        off = strlen(res);

        snprintf(res + off, sizeof(res) - off, "\n【1. 学习搭子】\n");
        off = strlen(res);
        if (strlen(me->study_subject) > 0) {
            snprintf(res + off, sizeof(res) - off, "  学习科目：%s\n", me->study_subject);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  年级/专业：%s\n", me->study_grade);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  期望科目：%s\n", me->exp_study_subject);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  期望年级：%s\n", me->exp_study_grade);
            off = strlen(res);
        }
        else snprintf(res + off, sizeof(res) - off, "  暂未填写\n");
        off = strlen(res);

        snprintf(res + off, sizeof(res) - off, "\n【2. 考研搭子】\n");
        off = strlen(res);
        if (strlen(me->postgrad_major) > 0) {
            snprintf(res + off, sizeof(res) - off, "  考研专业：%s\n", me->postgrad_major);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  目标院校：%s\n", me->postgrad_school);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  期望专业：%s\n", me->exp_postgrad_major);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  期望院校：%s\n", me->exp_postgrad_school);
            off = strlen(res);
        }
        else snprintf(res + off, sizeof(res) - off, "  暂未填写\n");
        off = strlen(res);

        snprintf(res + off, sizeof(res) - off, "\n【3. 兴趣交友】\n");
        off = strlen(res);
        if (strlen(me->friend_hobby) > 0) {
            snprintf(res + off, sizeof(res) - off, "  兴趣爱好：%s\n", me->friend_hobby);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  性格特点：%s\n", me->friend_personality);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  期望兴趣：%s\n", me->exp_friend_hobby);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  期望性格：%s\n", me->exp_friend_personality);
            off = strlen(res);
        }
        else snprintf(res + off, sizeof(res) - off, "  暂未填写\n");
        off = strlen(res);

        snprintf(res + off, sizeof(res) - off, "\n【4. 异性恋爱】\n");
        off = strlen(res);
        if (strlen(me->love_gender) > 0) {
            snprintf(res + off, sizeof(res) - off, "  性别：%s\n", me->love_gender);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  个人简介：%s\n", me->love_intro);
            off = strlen(res);
            snprintf(res + off, sizeof(res) - off, "  期望性别：%s\n", me->exp_love_gender);
            off = strlen(res);
        }
        else snprintf(res + off, sizeof(res) - off, "  暂未填写\n");

        write(cfd, res, strlen(res));
    }
    else if (strstr(cmd, "UPDATE:") == cmd) {
        int mode_to_update;
        char field1[100], field2[100];
        sscanf(cmd, "UPDATE:%d:%[^|]|%s", &mode_to_update, field1, field2);

        if (mode_to_update == 1) {
            strcpy(me->study_subject, field1);
            strcpy(me->study_grade, field2);
        }
        else if (mode_to_update == 2) {
            strcpy(me->postgrad_major, field1);
            strcpy(me->postgrad_school, field2);
        }
        else if (mode_to_update == 3) {
            strcpy(me->friend_hobby, field1);
            strcpy(me->friend_personality, field2);
        }
        else if (mode_to_update == 4) {
            strcpy(me->love_gender, field1);
            strcpy(me->love_intro, field2);
        }
        save_users();
        write(cfd, "个人信息已更新", strlen("个人信息已更新"));
    }
    else if (strstr(cmd, "SWITCH:") == cmd) {
        int new_mode = atoi(cmd + 7);
        bool info_filled = true;

        switch (new_mode) {
        case MODE_STUDY: info_filled = (strlen(me->study_subject) > 0); break;
        case MODE_POSTGRAD: info_filled = (strlen(me->postgrad_major) > 0); break;
        case MODE_FRIEND: info_filled = (strlen(me->friend_hobby) > 0); break;
        case MODE_LOVE: info_filled = (strlen(me->love_gender) > 0); break;
        default: info_filled = false; break;
        }

        if (!info_filled) {
            char res[100];
            snprintf(res, sizeof(res), "NEED_FILL:%d", new_mode);
            write(cfd, res, strlen(res));
        }
        else {
            me->current_mode = (MatchMode)new_mode;
            save_users();
            const char* mode_str;
            switch (me->current_mode) {
            case MODE_STUDY: mode_str = "学习搭子"; break;
            case MODE_POSTGRAD: mode_str = "考研搭子"; break;
            case MODE_FRIEND: mode_str = "兴趣交友"; break;
            case MODE_LOVE: mode_str = "异性恋爱"; break;
            default: mode_str = "未选择模式"; break;
            }
            char res[100];
            snprintf(res, sizeof(res), "模式切换成功！当前模式：%s", mode_str);
            write(cfd, res, strlen(res));
        }
    }
    else write(cfd, "未知指令", strlen("未知指令"));
}

void* handle_client(void* arg) {
    int cfd = *(int*)arg;
    free(arg);
    char buf[2048], id[20];
    read(cfd, id, sizeof(id));

    pthread_mutex_lock(&g_mutex);
    if (g_fd_cnt < MAX_USER) {
        g_client_fds[g_fd_cnt] = cfd;
        strcpy(g_client_ids[g_fd_cnt], id);
        g_fd_cnt++;
    }
    pthread_mutex_unlock(&g_mutex);

    User* me = find_user(id);
    if (!me) { close(cfd); return NULL; }

    memset(buf, 0, sizeof(buf));
    int offset = 0;
    snprintf(buf + offset, sizeof(buf) - offset, "MODE:%d\n", me->current_mode);
    offset = strlen(buf);

    int pending_cnt = 0;
    snprintf(buf + offset, sizeof(buf) - offset, "PENDING_LIST:\n");
    offset = strlen(buf);
    for (int i = 0; i < me->app_cnt; i++) {
        if (me->apps[i].status == 0) {
            snprintf(buf + offset, sizeof(buf) - offset, "APP:%s|%s\n", me->apps[i].from_id, me->apps[i].content);
            offset = strlen(buf);
            pending_cnt++;
        }
    }
    snprintf(buf + offset, sizeof(buf) - offset, "PENDING_CNT:%d\n", pending_cnt);
    write(cfd, buf, strlen(buf));

    while (1) {
        memset(buf, 0, sizeof(buf));
        int n = read(cfd, buf, sizeof(buf) - 1);
        if (n <= 0 || strcmp(buf, "quit") == 0) break;
        buf[n] = '\0';

        char* cmd = strtok(buf, "\n");
        while (cmd != NULL) {
            handle_single_cmd(cfd, me, id, cmd);
            cmd = strtok(NULL, "\n");
        }
    }

    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < g_fd_cnt; i++) {
        if (g_client_fds[i] == cfd) {
            for (int j = i; j < g_fd_cnt - 1; j++) {
                g_client_fds[j] = g_client_fds[j + 1];
                strcpy(g_client_ids[j], g_client_ids[j + 1]);
            }
            g_fd_cnt--; break;
        }
    }
    pthread_mutex_unlock(&g_mutex);

    close(cfd);
    return NULL;
}

int main() {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) sys_err("socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) sys_err("bind");
    if (listen(lfd, 100) == -1) sys_err("listen");

    load_users();
    std::cout << "✅ 服务器启动成功 端口：" << SERV_PORT << std::endl;

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t len = sizeof(cli_addr);
        int cfd = accept(lfd, (struct sockaddr*)&cli_addr, &len);
        if (cfd == -1) continue;

        pthread_t tid;
        int* p = (int*)malloc(sizeof(int));
        *p = cfd;
        pthread_create(&tid, NULL, handle_client, p);
        pthread_detach(tid);
    }

    close(lfd);
    return 0;
}