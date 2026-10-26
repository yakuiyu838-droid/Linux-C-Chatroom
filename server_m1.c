#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "Socket_server_init.h" 

#define BUF_SIZE 1024
#define MAX_CLIENTS FD_SETSIZE

int main() 
{
    int client_fd;
    // 这里只用管客户端地址，服务端地址已经被 Socket_init 封装好了
    struct sockaddr_in client_addr; 
    socklen_t client_addr_len;
    char buf[BUF_SIZE];
    ssize_t recv_len;

    // ================== 1. 直接调用封装好的初始化 ==================
    int port = 6666; // 你想用的端口
    int listen_fd = Socket_server_init(port); 
    if (listen_fd < 0) 
    {
        printf("服务端初始化失败\n");
        return 1;
    }
    printf("[服务端] 已启动，监听端口 %d,等待连接...\n", port);

    // ================== 2. 关键数据结构准备 ==================
    int client_fds[MAX_CLIENTS]; 
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        client_fds[i] = -1; 
    }
    int max_fd = listen_fd; 
    fd_set readfds;         

    // ================== 3. 主循环 ==================
    while (1) 
    {
        FD_ZERO(&readfds);                 
        FD_SET(listen_fd, &readfds);       

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_fds[i] != -1)
            {
                FD_SET(client_fds[i], &readfds); 
            }
        }

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) 
        {
            perror("select");
            break;
        }

        // ================== 4. 处理新客户端连接 ==================
        if (FD_ISSET(listen_fd, &readfds))
         {
            client_addr_len = sizeof(client_addr);
            client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_fd < 0) 
            {
                perror("accept");
                continue;
            }

            int slot = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) 
            {
                if (client_fds[i] == -1) 
                {
                    client_fds[i] = client_fd;
                    slot = i;
                    break;
                }
            }

            if (slot == -1)
             {
                printf("[服务端] 连接已满，拒绝 fd=%d\n", client_fd);
                close(client_fd);
            } 
            else 
            {
                if (client_fd > max_fd) 
                {
                  max_fd = client_fd; 
                }
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
                printf("[服务端] 新客户端 %s 已连接，fd=%d，分配至槽位 %d\n", ip, client_fd, slot);
            }
        }

        // ================== 5. 处理已连接的客户端 ==================
        for (int i = 0; i < MAX_CLIENTS; i++)
         {
            int fd = client_fds[i];
            
            if (fd == -1 || !FD_ISSET(fd, &readfds)) 
            {
                continue;
            }

            recv_len = recv(fd, buf, BUF_SIZE - 1, 0);

            if (recv_len > 0) 
            {
                buf[recv_len] = '\0';
                printf("[服务端] 收到 fd=%d 的消息: %s\n", fd, buf);
                send(fd, buf, recv_len, 0);
            } 
            else if (recv_len <= 0) 
            {
                printf("[服务端] 客户端 fd=%d 断开连接\n", fd);
                FD_CLR(fd, &readfds);
                close(fd);
                client_fds[i] = -1;

                if (fd == max_fd)
                 {
                    max_fd = listen_fd; 
                    for (int j = MAX_CLIENTS - 1; j >= 0; j--) 
                    {
                        if (client_fds[j] != -1) 
                        {
                            max_fd = client_fds[j];
                            break;
                        }
                    }
                }
            }
        }
    }

    close(listen_fd);
    printf("[服务端] 程序退出\n");
    return 0;
}