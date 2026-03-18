#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//服务器端口（必须和服务端完全一致）
#define SERV_PORT 9527

//错误处理函数
void sys_err(const char* str) {
    perror(str);
    exit(1);
}

int main() {
    //客户端socket的文件描述符
    int cfd;
    //服务器地址结构体
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    //服务器ip：127.0.0.1（本机测试用）
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    char buf[1024];//收发消息的缓冲区
    int n;//接受数据的长度

    //创建客户端socket
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) sys_err("客户端socket创建失败");

    //连接服务器
    if (connect(cfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        sys_err("连接服务器失败");

    // -------- 学号登录 --------
    std::cout << "请输入你的学号：";
    std::cin >> buf;
    //发送学号给服务器，完成登录
    write(cfd, buf, strlen(buf));
    std::cout << "✅ 学号 " << buf << " 登录成功！" << std::endl;

    // -------- 消息收发 --------
    while (1) {
        memset(buf, 0, sizeof(buf));
        std::cout << "请输入消息：（输入quit退出客户端）";
        std::cin >> buf;

        write(cfd, buf, strlen(buf));
        if (strcmp(buf, "quit") == 0) break;

        memset(buf, 0, sizeof(buf));
        n = read(cfd, buf, sizeof(buf));
        if (n <= 0) break;

        std::cout << "📢 服务器回显：" << buf << std::endl;
    }

    close(cfd);
    return 0;
}