#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// 服务器监听端口号
#define SERV_PORT 9527
// 最大支持在线用户数
#define MAX_USER 100

// 用户结构体
typedef struct {
    char id[20];// 用户唯一标识：学号
    int fd;// 客户端连接的socket描述符，用于收发消息
} User;

// 好友关系结构体
typedef struct {
    char a[20];// 好友一方的学号
    char b[20];// 好友另一方的学号
} FriendRelation;

// 离线好友申请结构体：存储未处理的好友申请
typedef struct {
    char from[20];   // 申请人的学号
    char to[20];     // 被申请人的学号
} OfflineRequest;

//好友关系数组，存储所有好友对
FriendRelation friend_list[MAX_USER * 2];
//好友关系总数
int friend_count = 0;

// 好友申请数组：存储所有未处理的申请
OfflineRequest request_list[MAX_USER * 2];
// 申请总数
int request_count = 0;

// 在线用户数组：存储当前所有在线的同学
User user_list[MAX_USER];
// 在线用户总数
int user_count = 0;
// 线程互斥锁：防止多线程同时修改全局数据，保证线程安全
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//---------------函数声明---------------------
// 处理客户端连接的线程函数
void* handle_client(void* arg);
// 错误处理函数
void sys_err(const char* str);
// 根据用户学号查找在线socket描述符
int find_user_fd(const char* id);
// 添加好友关系（内存+文件持久化）
void add_friend(const char* id1, const char* id2);
// 删除好友关系（内存+文件持久化）
void del_friend(const char* id1, const char* id2);
// 判断两个用户是否是好友
int is_friend(const char* id1, const char* id2);
// 从文件加载历史好友数据
void load_friends();
// 保存好友数据到文件
void save_friends();
// 从文件加载历史好友申请数据
void load_requests();
// 保存好友申请数据到文件
void save_requests();
// 用户上线时，推送所有未处理的好友申请
void send_offline_requests(int cfd, const char* user_id);
// 删除已处理的好友申请（同意/拒绝后调用）
void remove_request(const char* from, const char* to);

int main() {
    //启动的时候，加载本地文件的好友和历史好友申请数据
    load_friends();
    load_requests();

    int lfd;//监听socket描述符
    struct sockaddr_in serv_addr;//服务器地址结构
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t clie_addr_len = sizeof(serv_addr);
    pthread_t tid;//线程id

    //socket
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) sys_err("socket创建失败");

    //设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //bind
    if (bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        sys_err("bind绑定失败");

    //listen
    if (listen(lfd, 128) == -1) sys_err("listen监听失败");
    std::cout << "✅ 服务器启动成功！数据加载完成！\n" << std::endl;

    while (1) {
        int* cfd_ptr = new int;
        //accept
        *cfd_ptr = accept(lfd, (struct sockaddr*)&serv_addr, &clie_addr_len);
        if (*cfd_ptr == -1) { delete cfd_ptr; continue; }

        //创建多线程处理该客户端，实现多人同时在线
        pthread_create(&tid, NULL, handle_client, (void*)cfd_ptr);
        //线程分离：自动释放资源
        pthread_detach(tid);
    }
    close(lfd);
    return 0;
}

//---------------函数实现---------------------
// 判断两人是否是好友
//return 1=是好友    return 0=不是好友
int is_friend(const char* id1, const char* id2) {
    //加锁
    /*加锁原因：friend_list 是多个线程共享的全局资源
    ，如果多个线程同时调用这个函数，会导致数据覆盖/错乱。*/
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < friend_count; i++) {
        if ((strcmp(friend_list[i].a, id1) == 0 && strcmp(friend_list[i].b, id2) == 0) ||
            (strcmp(friend_list[i].a, id2) == 0 && strcmp(friend_list[i].b, id1) == 0)) {
            //找到后，解锁，返回1（是好友）
            pthread_mutex_unlock(&mutex);
            return 1;
        }
    }
    //解锁，返回0（不是好友）
    pthread_mutex_unlock(&mutex);
    return 0;
}

//从friends.txt加载所有好友数据，存入内存中friend_list
void load_friends() {
    //加锁
    pthread_mutex_lock(&mutex);
    //c式的文件读写
    //打开/创建名为friends.txt的文件，返回一个文件指针fp
    //a+:文件不存在则自动创建；存在则允许读和末尾追加写
    FILE* fp = fopen("friends.txt", "a+");
    //指针移动到文件开头
    rewind(fp);

    //定义临时缓冲区，临时存储从文件中读取到的一对好友id
    char a[20], b[20];
    //逐行读取好友对
    while (fscanf(fp, "%s %s", a, b) != EOF)//fscanf:从文件fp中按指定格式读取数据，存入变量a和b
    {
        strcpy(friend_list[friend_count].a, a);
        strcpy(friend_list[friend_count].b, b);
        friend_count++;
    }
    //关闭文件，释放系统分配的文件资源
    fclose(fp);
    //解锁
    pthread_mutex_unlock(&mutex);
}

// 保存好友
//保存好友数据到friends.txt
void save_friends() {
    //加锁，friend_list和friend_count是共享资源
    pthread_mutex_lock(&mutex);
    //w:如果文件不存在则自动创建；如果文件已经存在，文件内容会被清空，从头开始写入
    FILE* fp = fopen("friends.txt", "w");
    for (int i = 0; i < friend_count; i++)
        //格式化写入文件：
        //格式：“好友a 好友b\n”，每行存储一对好友关系
        //把friend_list中的第i条记录的a和b两个id，格式写入fp
        fprintf(fp, "%s %s\n", friend_list[i].a, friend_list[i].b);
    //关闭文件，释放分配的文件资源
    fclose(fp);
    //解锁
    pthread_mutex_unlock(&mutex);
}

// 加载离线申请
//从request.txt文件中读取所有未处理的好友申请，存入内存request_list
void load_requests() {
    //加锁，request_list和request_count是共享资源
    pthread_mutex_lock(&mutex);
    //a+:文件不存在则创建，文件存在则允许读和末尾追加写入
    FILE* fp = fopen("requests.txt", "a+");
    //复位文件指针到开头
    //因为a+模式打开后指针默认在文件末尾，直接读取会读不到内容，必须移到开头才能读取全部申请数据
    rewind(fp);
    //定义临时缓冲区
    //f:申请方id，t:接收方id
    char f[20], t[20];
    while (fscanf(fp, "%s %s", f, t) != EOF) {
        strcpy(request_list[request_count].from, f);
        strcpy(request_list[request_count].to, t);
        request_count++;
    }
    //关闭文件
    fclose(fp);
    //解锁
    pthread_mutex_unlock(&mutex);
}

// 保存离线申请
void save_requests() {
    //加锁，因为有共享资源request_list
    pthread_mutex_lock(&mutex);
    //w:文件不存在则创建，若存在则清空原有内容
    FILE* fp = fopen("requests.txt", "w");
    for (int i = 0; i < request_count; i++)
        // 将第 i 条申请的「发起者ID(from)」和「接收者ID(to)」以 "发起者 接收者\n" 的格式写入文件
        fprintf(fp, "%s %s\n", request_list[i].from, request_list[i].to);
    //关闭文件
    fclose(fp);
    //解锁
    pthread_mutex_unlock(&mutex);
}

// 添加好友
void add_friend(const char* id1, const char* id2) {
    //加锁，因为有共享资源friend_list
    pthread_mutex_lock(&mutex);
    strcpy(friend_list[friend_count].a, id1);
    strcpy(friend_list[friend_count].b, id2);
    friend_count++;
    //解锁
    pthread_mutex_unlock(&mutex);
    save_friends();//保存好友到friends.txt
}

// 删除好友
void del_friend(const char* id1, const char* id2) {
    //加锁
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < friend_count; i++) {
        if ((strcmp(friend_list[i].a, id1) == 0 && strcmp(friend_list[i].b, id2) == 0) ||
            (strcmp(friend_list[i].a, id2) == 0 && strcmp(friend_list[i].b, id1) == 0)) {
            //删除之后，因为用的是数组，后面的往前面移动
            for (int j = i; j < friend_count - 1; j++)
                friend_list[j] = friend_list[j + 1];
            friend_count--;
            break;
        }
    }
    //解锁
    pthread_mutex_unlock(&mutex);
    //保存好友到friends.txt
    save_friends();
}

//查找用户是否在线
int find_user_fd(const char* id) {
    int fd = -1;
    //加锁
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < user_count; i++) {
        //user_list：当前在线的用户
        if (strcmp(user_list[i].id, id) == 0) {
            fd = user_list[i].fd;
            break;
        }
    }
    //解锁
    pthread_mutex_unlock(&mutex);
    return fd;
}

// 上线推送所有未处理的申请
void send_offline_requests(int cfd, const char* user_id) {
    //加锁,request_count全局共享
    pthread_mutex_lock(&mutex);
    //request_count：记录requesr_count,request_list里面有多少个元素
    for (int i = 0; i < request_count; i++) {
        //request_list[i].to：接收好友申请的用户id
        if (strcmp(request_list[i].to, user_id) == 0) {
            //提示文本缓冲区
            char msg[256];
            //格式化消息内容
            // ✅ 修复1：添加 REJECT 拒绝提示
            snprintf(msg, sizeof(msg), "📩 未处理申请：用户%s想加你为好友！回复@%s:ACCEPT同意/REJECT拒绝", request_list[i].from, request_list[i].from);            //向客户端套接字写入格式化信息
            write(cfd, msg, strlen(msg));
        }
    }
    //解锁
    pthread_mutex_unlock(&mutex);
}

// 删除已处理的申请（同意/拒绝时调用）
void remove_request(const char* from, const char* to) {
    //加锁
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < request_count; i++) {
        if (strcmp(request_list[i].from, from) == 0 && strcmp(request_list[i].to, to) == 0) {
            //覆盖删除，后面的前移
            for (int j = i; j < request_count - 1; j++) {
                request_list[j] = request_list[j + 1];
            }
            request_count--;
            break;
        }
    }
    // 解锁
    pthread_mutex_unlock(&mutex);
    // 保存到friends.txt
    save_requests();
}

// 客户端处理线程函数：每个客户端连接都会创建一个线程执行此函数
void* handle_client(void* arg) {
    int cfd = *(int*)arg;
    //删除的时候一定要把指定无类型指针的类型
    delete (int*)arg;

    //缓冲区：buf接收客户端数据，user_id存储当前登录用户的学号
    char buf[1024], user_id[20];
    //n：read函数的返回值，记录读取到的字节数
    int n;

    // ---------登录--------------
    //清空缓存区
    memset(buf, 0, sizeof(buf));
    //从客户端socket读取数据，buf为传出参数
    n = read(cfd, buf, sizeof(buf));
    //读取失败/客户端断开连接：关闭socket，退出线程
    if (n <= 0) { close(cfd); return NULL; }
    //将客户端发送的学号复制到user_id，标记当前登录用户
    strcpy(user_id, buf);
    std::cout << "✅ 用户" << user_id << "上线\n";
    //----------登录.end-------------------

    //---------- 将用户加入在线用户列表---------------
    //加锁
    pthread_mutex_lock(&mutex);
    //user_list:在线用户列表
    strcpy(user_list[user_count].id, user_id);
    user_list[user_count].fd = cfd;
    user_count++;
    //解锁
    pthread_mutex_unlock(&mutex);
    //---------- 将用户加入在线用户列表.end---------------


    // 推送离线好友申请：用户上线后，发送所有未处理的申请
    send_offline_requests(cfd, user_id);

    //循环处理客户端发送的所有指令
    while (1) {
        //每次读取前清空缓冲区
        memset(buf, 0, sizeof(buf));
        //读取客户端消息
        n = read(cfd, buf, sizeof(buf));
        if (n <= 0) break;

        //在服务端打印日志：当前用户发送的内容
        std::cout << "用户" << user_id << "：" << buf << std::endl;

        //target_ied：目标用户id    cmd:指令/消息
        char target_id[20] = { 0 }, cmd[32] = { 0 };
        if (buf[0] == '@') {
            char* p = strchr(buf, ':');
            //if找到':',说明客户端输入的消息格式正确
            if (p) {
                *p = '\0';
                //提取目标用户id（@后面的内容）
                strcpy(target_id, buf + 1);
                //提取指令/消息内容（:后面的内容）
                strcpy(cmd, p + 1);

                // 1.MATCH指令：发送好友申请
                if (strcmp(cmd, "MATCH") == 0) {
                    //加锁
                    pthread_mutex_lock(&mutex);
                    //记录申请发起者
                    strcpy(request_list[request_count].from, user_id);
                    //记录申请接受者
                    strcpy(request_list[request_count].to, target_id);
                    request_count++;
                    //解锁
                    pthread_mutex_unlock(&mutex);
                    //保留申请，防止服务器重启丢失
                    save_requests();

                    // 如果对方在线，实时推送
                    int fd = find_user_fd(target_id);
                    if (fd != -1) {
                        char msg[256];
                        //提示信息
                        snprintf(msg, sizeof(msg), "📩 未处理申请：用户%s想加你为好友！回复@%s:ACCEPT同意/REJECT拒绝", user_id, user_id);
                        //向客户端套接字写入格式化信息
                        write(fd, msg, strlen(msg));
                    }
                    write(cfd, "✅ 好友申请已发送！对方上线即可查看", 40);
                }
                // 2. ACCEPT指令：同意好友（处理后删除申请）
                else if (strcmp(cmd, "ACCEPT") == 0) {
                    add_friend(user_id, target_id);
                    remove_request(target_id, user_id); // 删除申请
                    write(cfd, "✅ 同意成功！你们已是好友！", 30);
                }
                // 3. REJECT指令：拒绝好友
                else if (strcmp(cmd, "REJECT") == 0) {
                    remove_request(target_id, user_id); // 删除申请
                    write(cfd, "❌已拒绝该好友申请", 25);
                }
                // 4. DEL：删除好友
                else if (strcmp(cmd, "DEL") == 0) {
                    del_friend(user_id, target_id);
                    write(cfd, "🗑️ 删除好友成功！", 20);
                }
                // 5. 聊天消息（必须是好友才能发）
                else {
                    if (!is_friend(user_id, target_id)) {
                        write(cfd, "❌ 你们还不是好友，无法发送消息！", 40);
                        continue;
                    }
                    int fd = find_user_fd(target_id);
                    if (fd != -1) {
                        char msg[1024];
                        snprintf(msg, sizeof(msg), "[%s]: %s", user_id, cmd);
                        write(fd, msg, strlen(msg));
                    }
                    else {
                        write(cfd, "❌ 对方不在线", 20);
                    }
                }
            }
            //else:没有找到':' ，说明客户端输入的消息格式错误
            else write(cfd, "❌ 格式错误", 15);
        }
        else write(cfd, "❌ 格式错误", 15);
    }

    // ---------用户下线-------------
    //加锁
    pthread_mutex_lock(&mutex);
    //遍历在线列表，找到当前用户
    for (int i = 0; i < user_count; i++) {
        if (user_list[i].fd == cfd) {
            //后面的元素向前移动，覆盖当前用户的位置
            for (int j = i; j < user_count - 1; j++)
                user_list[j] = user_list[j + 1];
            user_count--;
            break;
        }
    }
    //解锁
    pthread_mutex_unlock(&mutex);

    //关闭客户端socket，释放资源
    close(cfd);
    //打印下线日志
    std::cout << "❌ 用户" << user_id << "下线\n";
    //线程结束
    return NULL;
}

void sys_err(const char* str) {
    perror(str);
    exit(1);
}