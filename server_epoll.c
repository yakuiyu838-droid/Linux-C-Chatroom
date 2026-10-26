#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "Socket_server_init.h"

#define MAX_CLIENTS 1024

int main()
{
    int listen_fd = Socket_server_init(6666);
    if (listen_fd < 0) return 1;
    printf("[服务端] 启动成功，epoll 监听端口 6666...\n");

    // 依然需要数组来维护所有在线客户端，为了群发广播
    int client_fds[MAX_CLIENTS];
    char client_names[MAX_CLIENTS][32];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_fds[i] = -1;
        memset(client_names[i], 0, sizeof(client_names[i]));
    }
    
    // 1. 创建个 epoll 实例，买个记事本
    int epfd = epoll_create(1);
    
    // 2. 把大门门铃贴上去
    struct epoll_event ev;
    ev.events = EPOLLIN; 
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    struct epoll_event events[1024];
    char buf[1024];

    while(1)
    {
        // 3. 躺平等，谁醒了给 n 赋值
        int n = epoll_wait(epfd, events, 1024, -1);
        if (n < 0)
        {
            perror("epoll_wait");
            break;
        }
        
        for(int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;
            
            // 情况 1：大门响了，新客户敲门
            if (fd == listen_fd)  // ⚠️ 已修复：==
            {
                struct sockaddr_in c_addr;
                socklen_t c_len = sizeof(c_addr);
                int c_fd = accept(listen_fd, (struct sockaddr*)&c_addr, &c_len);
                
                if (c_fd < 0)
                {
                    continue; 
                }
                printf("新客户端连入, fd = %d\n", c_fd);
                
                // 把新人的 fd 也登记到 epoll 记事本里
                struct epoll_event new_ev;
                new_ev.events = EPOLLIN;
                new_ev.data.fd = c_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, c_fd, &new_ev);
                
                // 找个空座位存进全局数组，方便以后广播
                for(int j = 0; j < MAX_CLIENTS; j++)
                {
                    if (client_fds[j] == -1)
                    {
                        client_fds[j] = c_fd;
                        break;
                    }
                }
            }
            // 情况 2：老客户发消息或者断开连接
            else
            {  
                int recv_len = recv(fd, buf, sizeof(buf) - 1, 0);
                
                if (recv_len > 0)
                {
                    buf[recv_len] = '\0';
                    printf("收到 fd=%d 的消息: %s\n", fd, buf);
                    
                    // 找到这个 fd 对应的座位，才能查到它的名字
                    int slot = -1;
                    for (int j = 0; j < MAX_CLIENTS; j++) 
                    {
                        if (client_fds[j] == fd) 
                        { 
                            slot = j; 
                            break;
                        }
                    }
                    
                    if (slot != -1) 
                    {
                        // NICK 判断
                        if (strncmp(buf, "NICK ", 5) == 0) 
                        {
                            strncpy(client_names[slot], buf + 5, sizeof(client_names[slot]) - 1);
                            client_names[slot][strcspn(client_names[slot], "\n")] = '\0';
                            
                            char sys_buf[128];
                            int sys_len = snprintf(sys_buf, sizeof(sys_buf), "[系统] %s 进入了聊天室\n", client_names[slot]);
                            
                            for (int j = 0; j < MAX_CLIENTS; j++) 
                            {
                                if (client_fds[j] != -1 && client_fds[j] != fd) 
                                {
                                    send(client_fds[j], sys_buf, sys_len, 0);
                                }
                            }
                        }
                        else
                        {
                            // 普通消息广播
                            char send_buf[2048];
                            int send_len = snprintf(send_buf, sizeof(send_buf), "%s: %s", client_names[slot], buf);
                            
                            for (int j = 0; j < MAX_CLIENTS; j++) 
                            {
                                if (client_fds[j] != -1 && client_fds[j] != fd) 
                                {
                                    send(client_fds[j], send_buf, send_len, 0);
                                }
                            }   
                        }
                    }
                }
                else // recv_len <= 0，断开连接
                {
                    printf("客户端断开连接， fd = %d\n", fd);
                    
                    // 从 epoll 移除（拆门铃）
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    
                    // 清理数组
                    for (int j = 0; j < MAX_CLIENTS; j ++)
                    {
                        if (client_fds[j] == fd) 
                        {
                            client_fds[j] = -1;
                            memset(client_names[j], 0, sizeof(client_names[j]));
                            break;
                        } 
                    }
                }
            }
        }
    }
    
    close(listen_fd);
    return 0;
}