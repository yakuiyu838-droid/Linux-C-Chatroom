#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>     // 必须引入 pthread 库
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Socket_client_init.h" // 你的封装函数

#define SERVER_IP "127.0.0.1"
#define PORT 6666
#define BUF_SIZE 1024

// ================== 接收消息的子线程 ==================
void *receive_thread(void *arg) 
{
    int client_fd = *(int *)arg;
    char buf[BUF_SIZE];
    int recv_len;

    while (1) 
    {
        // 在这里死等，只要服务端（或别人）发消息过来，立刻就会醒
        recv_len = recv(client_fd, buf, BUF_SIZE - 1, 0);
        
        if (recv_len > 0) 
        {
            buf[recv_len] = '\0';
            
            // ⚠️ 解决粘包：按 \n 切割收到的数据
            char *msg = strtok(buf, "\n");
            while (msg != NULL) 
            {
                printf("\n[新消息] %s\n> ", msg);
                fflush(stdout); // 刷新屏幕，让光标重新回到 > 后面
                msg = strtok(NULL, "\n");
            }
        } 
        else if (recv_len == 0) 
        {
            printf("\n[客户端] 服务端已断开连接\n");
            exit(0); // 服务端挂了，直接退出客户端
        } 
        else 
        {
            perror("recv");
            exit(1);
        }
    }
    return NULL;
}

// ================== 主线程：专门发消息 ==================
int main() 
{
    char buf[BUF_SIZE];
    int send_len;

    printf("[客户端] 正在连接服务端 %s:%d...\n", SERVER_IP, PORT);
    int client_fd = Socket_client_init(SERVER_IP, PORT);
    if (client_fd < 0)
    {
      return 1;
    } 
    
    printf("[客户端] 已成功连接到服务端！\n");

    // 1. 创建接收消息的子线程
    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_thread, &client_fd) != 0) 
    {
        perror("pthread_create");
        return 1;
    }
    pthread_detach(tid); // 线程分离，让它自己在后台跑

    // 2. 主线程死循环：专门读取键盘并发送
    while (1) 
    {
        printf("> ");
        fflush(stdout);

        // 如果按了 Ctrl+D 结束输入
        if (fgets(buf, BUF_SIZE, stdin) == NULL) break;

        // ⚠️ 解决粘包：直接用 strncmp 比较，不要删除 \n！
        if (strncmp(buf, "exit", 4) == 0 || strncmp(buf, "quit", 4) == 0) 
        {
            break;
        }

        // 发送。注意这里发送的长度是 strlen(buf)，保留了 \n 作为边界！
        send_len = send(client_fd, buf, strlen(buf), 0);
        if (send_len < 0) 
        {
            perror("send");
            break;
        }
    }

    close(client_fd);
    printf("\n[客户端] 程序退出\n");
    return 0;
}