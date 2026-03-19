#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERV_PORT 9527
#define BUF_SIZE 1024

// 全局变量：客户端套接字（供子线程访问）
int cfd;

// 错误处理函数
void sys_err(const char* str) {
    perror(str);
    exit(EXIT_FAILURE);
}

// 子线程函数：专门接收服务器消息（实时推送）
void* recv_thread(void* arg) {
    char buf[BUF_SIZE];
    while (true) {
        memset(buf, 0, BUF_SIZE);
        int n = read(cfd, buf, BUF_SIZE);
        if (n <= 0) {
            std::cout << "\n❌ 服务器连接断开，按回车退出\n";
            close(cfd);
            exit(EXIT_FAILURE);
        }
        // 实时打印消息，覆盖当前输入行
        std::cout << "\n📢 系统通知：" << buf << "\n";
        std::cout << "输入指令：";
        fflush(stdout); // 强制刷新，保证提示语显示
    }
    return NULL;
}

int main() {
    // 1. 创建 TCP 套接字
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) sys_err("socket failed");

    // 2. 连接服务器（上线时替换为服务器公网 IP）
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(cfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        sys_err("connect failed");

    // 3. 学号登录
    char buf[BUF_SIZE];
    std::cout << "请输入你的学号：";
    std::cin >> buf;
    write(cfd, buf, strlen(buf));
    std::cout << "✅ 登录成功！实时消息模式已启动\n\n";

    // 4. 打印使用说明
    std::cout << "===== 校园交友系统（正式上线版）=====\n";
    std::cout << "• 加好友：@目标学号:MATCH\n";
    std::cout << "• 同意好友：@邀请者学号:ACCEPT\n";
    std::cout << "• 拒绝好友：@邀请者学号:REJECT\n";
    std::cout << "• 删除好友：@好友学号:DEL\n";
    std::cout << "• 聊天：@好友学号:消息内容\n";
    std::cout << "• 退出：quit\n";
    std::cout << "===================================\n\n";

    // 5. 创建接收消息的子线程（核心：收发分离）
    pthread_t tid;
    if (pthread_create(&tid, NULL, recv_thread, NULL) != 0)
        sys_err("pthread_create failed");
    pthread_detach(tid); // 子线程分离，自动释放资源

    // 6. 主线程：处理用户输入与发送指令
    std::cout << "输入指令：";
    fflush(stdout);
    while (true) {
        memset(buf, 0, BUF_SIZE);
        fgets(buf, BUF_SIZE, stdin);
        buf[strcspn(buf, "\n")] = '\0'; // 去掉换行符

        if (strlen(buf) == 0) { // 空输入，重新提示
            std::cout << "输入指令：";
            fflush(stdout);
            continue;
        }

        // 退出指令
        if (strcmp(buf, "quit") == 0) {
            write(cfd, buf, strlen(buf));
            std::cout << "👋 已退出登录\n";
            close(cfd);
            break;
        }

        // 发送指令给服务器
        write(cfd, buf, strlen(buf));
        std::cout << "输入指令：";
        fflush(stdout);
    }

    return 0;
}