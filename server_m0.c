#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>     // 多线程核心
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUF_SIZE 1024

// 每个客户端的专属服务线程
void *handle_client(void *arg) 
{
    // 1. 取出主线程传过来的 fd
    int client_fd = *(int *)arg;
    free(arg); // 释放主线程 malloc 的内存

    char buf[BUF_SIZE];
    ssize_t recv_len;

    // 2. 获取客户端 IP
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN] = "unknown";
    if (getpeername(client_fd, (struct sockaddr *)&client_addr, &addr_len) == 0) {
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    }

    printf("[服务端] 线程 %lu 开始服务客户端 %s\n", (unsigned long)pthread_self(), client_ip);

    // 3. 循环接收和回显
    while (1) {
        recv_len = recv(client_fd, buf, BUF_SIZE - 1, 0);
        if (recv_len > 0) {
            buf[recv_len] = '\0';
            printf("[服务端] 收到来自 %s 的消息: %s\n", client_ip, buf);
            send(client_fd, buf, recv_len, 0);
        } 
        else if (recv_len == 0) {
            printf("[服务端] 客户端 %s 断开连接\n", client_ip);
            break;
        } 
        else {
            perror("recv");
            break;
        }
    }

    // 4. 释放资源
    close(client_fd);
    printf("[服务端] 线程 %lu 结束服务\n", (unsigned long)pthread_self());
    return NULL;
}

int main() 
{
    int listen_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    pthread_t tid;

    // 1. 初始化监听套接字
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); close(listen_fd); return 1;
    }
    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("listen"); close(listen_fd); return 1;
    }
    printf("[服务端] 已启动（多线程 M0），监听端口 %d...\n", PORT);

    // 2. 主循环：无限接客，每接一个就派一个线程
    while (1) {
        client_addr_len = sizeof(client_addr);
        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0) { perror("accept"); continue; }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("[服务端] 新客户端 %s 已连接，fd=%d\n", client_ip, client_fd);

        // 把 fd 拷贝到堆上再传参，防止被覆盖
        int *pfd = malloc(sizeof(int));
        if (pfd == NULL) { close(client_fd); continue; }
        *pfd = client_fd;

        // 创建专属线程
        if (pthread_create(&tid, NULL, handle_client, pfd) != 0) {
            perror("pthread_create");
            close(client_fd); free(pfd);
            continue;
        }
        
        // 分离线程，让它自生自灭，主线程不用管
        pthread_detach(tid);
    }

    close(listen_fd);
    return 0;
}