// 标准C++头文件，兼容C语言所有系统API
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// 服务器监听端口（校园交友系统固定端口）
#define SERV_PORT 9527
// 最大支持的在线用户数量
#define MAX_USER 100

// 用户结构体：存储在线用户信息（C/C++通用）
typedef struct {
    char id[20];   // 学号
    int fd;        // 客户端socket
} User;

//在线用户列表：储存所有连接到服务器的用户
User user_list[MAX_USER];
int user_count = 0;
// 互斥锁：保护共享数据（和C语言用法一样）
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//线程处理函数：一个客户开一个线程
void* handle_client(void* arg);
//统一错误处理函数
void sys_err(const char* str);

// 主函数：服务器入口
int main() {
    //文件描述符
    int lfd;
    //地址结构体
    //这里面用的ip和port都是网络的，要转一下
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;//ipv4
    serv_addr.sin_port = htons(SERV_PORT);//host->net
    //服务端这里用INADDR_ANY,让服务器绑定本机所有ip，不管谁用哪个ip连，都能连接成功
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//host->net
    //客户端地址长度
    socklen_t clie_addr_len = sizeof(serv_addr);
    //线程id：用于创建新线程
    pthread_t tid;

    //socket
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) sys_err("socket创建失败");

    // 端口复用：解决端口占用问题
    //只用确定lfd这个sockfd，其他的都是固定写法
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 4. 绑定IP和端口到监听Socket 返回值：成功：0，失败：-1
    if (bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        sys_err("bind绑定失败");

    // 3. 监听客户端（最大等待数列128）
    if (listen(lfd, 128) == -1) sys_err("listen监听失败");
    std::cout << "✅ 校园交友服务器启动成功！端口：" << SERV_PORT << "，等待用户连接..." << std::endl;

    // 4. 循环接收客户端（多线程）
    while (1) {
        //服务器同时服务多个客户端，他的作用是从内核队列中取出一个新的客户端连接，因此必须写在while循环的里面
        //accept返回值：一个与客户端成功连接的socket文件描述符
        int* cfd_ptr = new int;
        *cfd_ptr = accept(lfd, (struct sockaddr*)&serv_addr, &clie_addr_len);

        if (*cfd_ptr == -1) {
            perror("accept失败");
            delete cfd_ptr;  // C++ 释放内存
            continue;
        }

        std::cout << "🔗 新用户连接，socket=" << *cfd_ptr << std::endl;
        //创建子线程：单独处理这个客户端
        pthread_create(&tid, NULL, handle_client, (void*)cfd_ptr);
        // 线程分离：线程结束后自动释放资源，无需手动回收
        pthread_detach(tid);
    }

    close(lfd);
    return 0;
}

// 线程函数：处理单个客户端
//void* 无类型指针/通用指针
void* handle_client(void* arg) {
    // 解引用获取客户端fd
    int cfd = *(int*)arg;
    //delete必须知道指针指向的具体类型，才能正确的释放内存
    //原因也不难理解，不同的数据类型占用的内存也不同呗
    delete (int*)arg;  //释放new的内存

    char buf[1024];//数据缓存区：存放收发的信息
    int n;//接受数据的长度
    char user_id[20];//存储当前用户的学号

    // -------- 接收学号，登录 --------
    //清空缓存区
    memset(buf, 0, sizeof(buf));
    //read返回值：>0(实际读取到的字节数) 0(读到eof，表示文件读完了) -1(失败)
    //读取客户端发送的学号
    n = read(cfd, buf, sizeof(buf));
    if (n <= 0) {
        close(cfd);
        return NULL;
    }
    //复制buf里面的学号到user_id中
    strcpy(user_id, buf);
    std::cout << "🎉 用户 " << user_id << " 登录成功！socket=" << cfd << std::endl;

    // -------- 加入在线用户列表（加锁保护） --------
    //加锁
    pthread_mutex_lock(&mutex);
    //储存用户学号  char[]类型的，所以要用strcpy
    strcpy(user_list[user_count].id, user_id);
    //储存客户端的sockfd
    user_list[user_count].fd = cfd;
    //在线用户++
    user_count++;
    //解锁
    pthread_mutex_unlock(&mutex);

    // -------- 循环处理消息 --------
    while (1) {
        memset(buf, 0, sizeof(buf));
        //读取客户端发送的消息
        //read会一直卡在这里等，当客户端断开了连接或者连接出错了，n<=0
        n = read(cfd, buf, sizeof(buf));

        // 客户端断开连接
        if (n <= 0) {
            std::cout << "❌ 用户 " << user_id << " 断开连接，socket=" << cfd << std::endl;

            //加锁
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < user_count; i++) {
                //找到用户当前的位置
                if (user_list[i].fd == cfd) {
                    //后面的用户向前覆盖，删除当前用户
                    for (int j = i; j < user_count - 1; j++)
                        user_list[j] = user_list[j + 1];
                    user_count--;//在线用户数-1
                    break;
                }
            }
            //解锁
            pthread_mutex_unlock(&mutex);

            close(cfd);
            break;
        }

        // 打印消息
        std::cout << "📩 收到用户 " << user_id << " 的消息：" << buf << std::endl;



        //--------------添加一对一聊天功能-----------------------

        //定义两个字符数组，用于存储解析后的内容
        char target_id[20] = { 0 };  // 目标用户的学号（要发送信息给谁）
        char msg[1024] = { 0 };      // 真正的消息内容

        //判断消息是否是以@开头，是则按转发规则处理
        if (buf[0] == '@') {
            //查找消息中第一个：的位置，用来分割学号和消息
            //strchr函数：在字符串中查找某个字符，找到返回地址(所以用指针指向他)，没找到返回NULL
            char* p = strchr(buf, ':');
            //找到了：，格式正确，开始解析
            if (p != NULL) {
                //把冒号改成字符串结束符\0，把原消息切成两段：@学号 和 消息内容
                *p = '\0';
                //strcpy(目标,复制的东西)
                //把*p改成了\0 那么读到这里的时候就会断开
                //，因此这个就复制的是id
                strcpy(target_id, buf + 1);
                //复制要发送的消息
                strcpy(msg, p + 1);


                // 拼接最终要发送给对方的消息格式：[发送者学号]:消息内容
                char send_buf[1024];
                size_t prefix_len = 1 + strlen(user_id) + 3; // "[user_id]: " 总长度
                size_t max_msg_len = sizeof(send_buf) - prefix_len - 1;

                // 手动截断 msg（可选，和%.*s配合更保险）
                if (strlen(msg) > max_msg_len) {
                    msg[max_msg_len] = '\0';
                }
                //用 %.*s 显式限制 msg 输出长度
                snprintf(send_buf, sizeof(send_buf), "[%s]: %.*s", user_id, (int)max_msg_len, msg);


                //去在线用户列表中查找目标用户是否在线
                int target_fd = -1;//目标用户的sockfd，默认-1表示不在线
                //加锁
                pthread_mutex_lock(&mutex);
                for (int i = 0; i < user_count; i++)
                {
                    //strcmp:字典序+ASCII码 <0:str1<str2 =0:str1=str2 >0:str1>str2
                    if (strcmp(user_list[i].id, target_id) == 0)
                    {
                        //找到了，记录目标用户的sockfd
                        target_fd = user_list[i].fd;
                        break;
                    }
                }
                //解锁
                pthread_mutex_unlock(&mutex);

                //如果找到了目标用户（在线）
                if (target_fd != -1)
                {
                    //把消息发送给目标客户端
                    write(target_fd, send_buf, strlen(send_buf));
                    std::cout << "✅ 转发给 " << target_id << " 成功！" << std::endl;
                }
                else
                {
                    // 目标用户不在线，给发送方返回错误提示
                    write(cfd, "❌ 对方不在线", strlen("❌ 对方不在线"));
                    std::cout << "❌ 用户：" << target_id << " 不在线" << std::endl;
                }
            }
            else
            {
                //没有找到冒号，消息格式错误
                write(cfd, "❌ 格式错误：@学号:消息", strlen("❌ 格式错误：@学号:消息"));
            }
        }
        else
        {
            // 9. 没有找到冒号，消息格式错误
            write(cfd, "❌ 格式错误，请使用：@学号:消息内容", strlen("❌ 格式错误，请使用：@学号:消息内容"));
        }

        //------------------添加一对一聊天功能----------------------------------

    }
    return NULL;
}

// 错误处理函数
//很简单的一个函数：输出哪里错误了，结束程序
void sys_err(const char* str) {
    perror(str);
    exit(1);
}