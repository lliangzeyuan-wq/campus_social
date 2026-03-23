#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <limits>

#define SERV_PORT 9527

typedef enum {
    MODE_NONE = 0,
    MODE_STUDY = 1,
    MODE_POSTGRAD = 2,
    MODE_FRIEND = 3,
    MODE_LOVE = 4
} MatchMode;

void sys_err(const char* str) { perror(str); exit(1); }
void clear_input_after_cin() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int cfd_global; // 全局socket，避免递归传参

void send_cmd(const char* cmd) {
    char buf[1024];
    write(cfd_global, cmd, strlen(cmd));
    write(cfd_global, "\n", 1);
    memset(buf, 0, sizeof(buf));
    read(cfd_global, buf, sizeof(buf) - 1);
    std::cout << "📌 " << buf << std::endl;
}

void handle_mode_fill(int mode_to_fill) {
    std::cout << "\n⚠️ 该模式未填写个人信息，请先完成填写！\n";
    char cmd[2048];

    if (mode_to_fill == 1) {
        char s[50], g[20];
        std::cout << "学习科目：";
        std::cin.getline(s, 50);
        std::cout << "年级/专业：";
        std::cin.getline(g, 20);
        snprintf(cmd, sizeof(cmd), "STUDY:%s|%s", s, g);
        send_cmd(cmd);
    }
    else if (mode_to_fill == 2) {
        char m[50], s[50];
        std::cout << "考研专业：";
        std::cin.getline(m, 50);
        std::cout << "目标院校：";
        std::cin.getline(s, 50);
        snprintf(cmd, sizeof(cmd), "POSTGRAD:%s|%s", m, s);
        send_cmd(cmd);
    }
    else if (mode_to_fill == 3) {
        char h[50], p[20];
        std::cout << "兴趣爱好：";
        std::cin.getline(h, 50);
        std::cout << "性格特点：";
        std::cin.getline(p, 20);
        snprintf(cmd, sizeof(cmd), "FRIEND:%s|%s", h, p);
        send_cmd(cmd);
    }
    else if (mode_to_fill == 4) {
        char g[10], i[100];
        std::cout << "性别：";
        std::cin.getline(g, 10);
        std::cout << "个人简介：";
        std::cin.getline(i, 100);
        snprintf(cmd, sizeof(cmd), "LOVE:%s|%s", g, i);
        send_cmd(cmd);
    }

    // 填写完成后直接切换模式
    snprintf(cmd, sizeof(cmd), "SWITCH:%d", mode_to_fill);
    send_cmd(cmd);
    std::cout << "✅ 信息填写完成，模式切换成功！\n";
}

int main() {
    int cfd;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    char buf[2048];
    int n;

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) sys_err("socket创建失败");
    cfd_global = cfd; // 赋值给全局变量

    if (connect(cfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        sys_err("连接服务器失败");

    // 登录逻辑
    std::cout << "请输入你的学号：";
    std::cin >> buf;
    write(cfd, buf, strlen(buf));
    std::cout << "✅ 学号 " << buf << " 登录成功！" << std::endl;
    clear_input_after_cin();

    // 接收登录返回的信息
    memset(buf, 0, sizeof(buf));
    n = read(cfd, buf, sizeof(buf) - 1);
    buf[n] = '\0';

    int user_mode = MODE_NONE;
    bool has_info = false;
    char pending_apps[50][2][100];
    int pending_cnt = 0;

    char* line = strtok(buf, "\n");
    while (line != NULL) {
        if (strstr(line, "MODE:") == line) {
            user_mode = atoi(line + 5);
            if (user_mode != MODE_NONE) has_info = true;
        }
        else if (strstr(line, "APP:") == line) {
            sscanf(line, "APP:%[^|]|%s", pending_apps[pending_cnt][0], pending_apps[pending_cnt][1]);
            pending_cnt++;
        }
        else if (strstr(line, "PENDING_CNT:") == line) {
            pending_cnt = atoi(line + 12);
        }
        line = strtok(NULL, "\n");
    }

    // 处理未读好友申请
    if (pending_cnt > 0) {
        std::cout << "\n📢 你有 " << pending_cnt << " 条未处理的好友申请：\n";
        for (int i = 0; i < pending_cnt; i++) {
            std::cout << "\n--- 申请" << i + 1 << " ---\n";
            std::cout << "来自用户：" << pending_apps[i][0] << "\n";
            std::cout << "申请内容：" << pending_apps[i][1] << "\n";
            std::cout << "请选择操作：1.同意  2.拒绝  3.稍后处理\n";

            int opt;
            std::cin >> opt;
            clear_input_after_cin();
            if (opt == 1) {
                char cmd[100];
                snprintf(cmd, sizeof(cmd), "AGREE:%s", pending_apps[i][0]);
                send_cmd(cmd);
            }
            else if (opt == 2) {
                char cmd[100];
                snprintf(cmd, sizeof(cmd), "REFUSE:%s", pending_apps[i][0]);
                send_cmd(cmd);
            }
            else {
                std::cout << "ℹ️ 已跳过该申请，可后续处理\n";
            }
        }
    }

    // 新用户首次填写信息
    int mode_choice = user_mode;
    if (!has_info) {
        while (true) {
            std::cout << "\n========== 校园多元社交匹配系统 ==========\n";
            std::cout << "1. 学习搭子\n";
            std::cout << "2. 考研搭子\n";
            std::cout << "3. 兴趣交友\n";
            std::cout << "4. 异性恋爱\n";
            std::cout << "=========================================\n";
            std::cout << "请输入选项（1/2/3/4）：";

            if (!(std::cin >> mode_choice)) {
                std::cin.clear();
                clear_input_after_cin();
                std::cout << "❌ 错误！请输入数字 1/2/3/4！" << std::endl;
                continue;
            }
            clear_input_after_cin();

            char cmd[100];
            snprintf(cmd, sizeof(cmd), "MODE:%d", mode_choice);
            send_cmd(cmd);
            break;
        }

        std::cout << "\n请填写个人信息：\n";
        char cmd[2048];
        if (mode_choice == 1) {
            char subject[50], grade[20];
            std::cout << "学习科目：";
            std::cin.getline(subject, 50);
            std::cout << "年级/专业：";
            std::cin.getline(grade, 20);
            snprintf(cmd, sizeof(cmd), "STUDY:%s|%s", subject, grade);
            send_cmd(cmd);
        }
        else if (mode_choice == 2) {
            char major[50], school[50];
            std::cout << "考研专业：";
            std::cin.getline(major, 50);
            std::cout << "目标院校：";
            std::cin.getline(school, 50);
            snprintf(cmd, sizeof(cmd), "POSTGRAD:%s|%s", major, school);
            send_cmd(cmd);
        }
        else if (mode_choice == 3) {
            char hobby[50], personality[20];
            std::cout << "兴趣爱好：";
            std::cin.getline(hobby, 50);
            std::cout << "性格特点：";
            std::cin.getline(personality, 20);
            snprintf(cmd, sizeof(cmd), "FRIEND:%s|%s", hobby, personality);
            send_cmd(cmd);
        }
        else if (mode_choice == 4) {
            char gender[10], intro[100];
            std::cout << "性别：";
            std::cin.getline(gender, 10);
            std::cout << "个人简介：";
            std::cin.getline(intro, 100);
            snprintf(cmd, sizeof(cmd), "LOVE:%s|%s", gender, intro);
            send_cmd(cmd);
        }

        std::cout << "\n请填写期望匹配对象信息：\n";
        if (mode_choice == 1) {
            char s[50], g[20];
            std::cout << "期望科目：";
            std::cin.getline(s, 50);
            std::cout << "期望年级：";
            std::cin.getline(g, 20);
            snprintf(cmd, sizeof(cmd), "EXP:%s|%s", s, g);
            send_cmd(cmd);
        }
        else if (mode_choice == 2) {
            char m[50], s[50];
            std::cout << "期望专业：";
            std::cin.getline(m, 50);
            std::cout << "期望院校：";
            std::cin.getline(s, 50);
            snprintf(cmd, sizeof(cmd), "EXP:%s|%s", m, s);
            send_cmd(cmd);
        }
        else if (mode_choice == 3) {
            char h[50], p[20];
            std::cout << "期望兴趣：";
            std::cin.getline(h, 50);
            std::cout << "期望性格：";
            std::cin.getline(p, 20);
            snprintf(cmd, sizeof(cmd), "EXP:%s|%s", h, p);
            send_cmd(cmd);
        }
        else {
            char g[10];
            std::cout << "期望性别：";
            std::cin.getline(g, 10);
            snprintf(cmd, sizeof(cmd), "EXP:%s", g);
            send_cmd(cmd);
        }
    }
    else {
        std::cout << "ℹ️ 检测到你已填写过个人信息，直接进入功能菜单\n";
    }

    // 功能菜单
    std::cout << "=====================================================\n";
    std::cout << "               🔥 功能使用说明 🔥                \n";
    std::cout << " 1. 智能匹配：输入 MATCH                          \n";
    std::cout << " 2. 发送申请：@对方学号:REQUEST:申请内容          \n";
    std::cout << " 3. 同意申请：AGREE:对方学号                       \n";
    std::cout << " 4. 拒绝申请：REFUSE:对方学号                      \n";
    std::cout << " 5. 好友聊天：@对方学号:消息内容                   \n";
    std::cout << " ---------- 新增功能 ----------                      \n";
    std::cout << " 6. 查询当前模式：输入 MODE                        \n";
    std::cout << " 7. 查询所有信息：输入 INFO                        \n";
    std::cout << " 8. 修改信息：输入 UPDATE                          \n";
    std::cout << " 9. 切换模式：输入 SWITCH                          \n";
    std::cout << " 10. 退出客户端：quit                              \n";
    std::cout << "=====================================================\n\n";

    std::cout << "请输入指令：";
    std::cout.flush();

    // select 循环
    fd_set fds;
    int max_fd = (cfd > STDIN_FILENO) ? cfd : STDIN_FILENO;

    while (1) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(cfd, &fds);

        int ret = select(max_fd + 1, &fds, NULL, NULL, NULL);
        if (ret < 0) break;

        // 处理服务器消息
        if (FD_ISSET(cfd, &fds)) {
            memset(buf, 0, sizeof(buf));
            n = read(cfd, buf, sizeof(buf) - 1);
            if (n <= 0) {
                std::cout << "\n❌ 与服务器断开连接！" << std::endl;
                break;
            }
            buf[n] = '\0';

            // 处理未填写模式的提示
            if (strstr(buf, "NEED_FILL:") == buf) {
                int mode_to_fill = atoi(buf + 9);
                handle_mode_fill(mode_to_fill);
                std::cout << "请输入指令：";
                std::cout.flush();
                continue;
            }

            std::cout << "\n📢 系统通知：\n" << buf << std::endl;
            std::cout << "请输入指令：";
            std::cout.flush();
        }

        // 处理用户输入
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            memset(buf, 0, sizeof(buf));
            std::cin.getline(buf, sizeof(buf));

            if (strcmp(buf, "quit") == 0) {
                write(cfd, "quit\n", 5);
                break;
            }
            else if (strcmp(buf, "MODE") == 0) {
                send_cmd("QUERY_MODE");
            }
            else if (strcmp(buf, "INFO") == 0) {
                send_cmd("QUERY_ALL_INFO");
            }
            else if (strcmp(buf, "UPDATE") == 0) {
                std::cout << "\n请选择要修改的模式：\n";
                std::cout << "1. 学习搭子\n";
                std::cout << "2. 考研搭子\n";
                std::cout << "3. 兴趣交友\n";
                std::cout << "4. 异性恋爱\n";
                std::cout << "请输入选项：";

                int m_choice;
                std::cin >> m_choice;
                clear_input_after_cin();

                char cmd[2048];
                if (m_choice == 1) {
                    char s[50], g[20];
                    std::cout << "学习科目：";
                    std::cin.getline(s, 50);
                    std::cout << "年级/专业：";
                    std::cin.getline(g, 20);
                    snprintf(cmd, sizeof(cmd), "UPDATE:1:%s|%s", s, g);
                    send_cmd(cmd);

                    char es[50], eg[20];
                    std::cout << "期望科目：";
                    std::cin.getline(es, 50);
                    std::cout << "期望年级：";
                    std::cin.getline(eg, 20);
                    snprintf(cmd, sizeof(cmd), "EXP:%s|%s", es, eg);
                    send_cmd(cmd);
                    std::cout << "✅ 学习信息更新成功！\n";
                }
                else if (m_choice == 2) {
                    char m[50], s[50];
                    std::cout << "考研专业：";
                    std::cin.getline(m, 50);
                    std::cout << "目标院校：";
                    std::cin.getline(s, 50);
                    snprintf(cmd, sizeof(cmd), "UPDATE:2:%s|%s", m, s);
                    send_cmd(cmd);

                    char em[50], es[50];
                    std::cout << "期望专业：";
                    std::cin.getline(em, 50);
                    std::cout << "期望院校：";
                    std::cin.getline(es, 50);
                    snprintf(cmd, sizeof(cmd), "EXP:%s|%s", em, es);
                    send_cmd(cmd);
                    std::cout << "✅ 考研信息更新成功！\n";
                }
                else if (m_choice == 3) {
                    char h[50], p[20];
                    std::cout << "兴趣爱好：";
                    std::cin.getline(h, 50);
                    std::cout << "性格特点：";
                    std::cin.getline(p, 20);
                    snprintf(cmd, sizeof(cmd), "UPDATE:3:%s|%s", h, p);
                    send_cmd(cmd);

                    char eh[50], ep[20];
                    std::cout << "期望兴趣：";
                    std::cin.getline(eh, 50);
                    std::cout << "期望性格：";
                    std::cin.getline(ep, 20);
                    snprintf(cmd, sizeof(cmd), "EXP:%s|%s", eh, ep);
                    send_cmd(cmd);
                    std::cout << "✅ 交友信息更新成功！\n";
                }
                else if (m_choice == 4) {
                    char g[10], i[100];
                    std::cout << "性别：";
                    std::cin.getline(g, 10);
                    std::cout << "个人简介：";
                    std::cin.getline(i, 100);
                    snprintf(cmd, sizeof(cmd), "UPDATE:4:%s|%s", g, i);
                    send_cmd(cmd);

                    char eg[10];
                    std::cout << "期望性别：";
                    std::cin.getline(eg, 10);
                    snprintf(cmd, sizeof(cmd), "EXP:%s", eg);
                    send_cmd(cmd);
                    std::cout << "✅ 恋爱信息更新成功！\n";
                }
            }
            else if (strcmp(buf, "SWITCH") == 0) {
                std::cout << "\n请选择要切换到的模式：\n";
                std::cout << "1. 学习搭子\n";
                std::cout << "2. 考研搭子\n";
                std::cout << "3. 兴趣交友\n";
                std::cout << "4. 异性恋爱\n";
                std::cout << "请输入选项：";

                int s_choice;
                std::cin >> s_choice;
                clear_input_after_cin();

                char cmd[100];
                snprintf(cmd, sizeof(cmd), "SWITCH:%d", s_choice);
                send_cmd(cmd);
            }
            else {
                send_cmd(buf);
            }

            std::cout << "请输入指令：";
            std::cout.flush();
        }
    }

    close(cfd);
    return 0;
}