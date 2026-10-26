#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <signal.h>

#include "Socket_server_init.h"

#define MAX_CLIENTS 1024

// 优雅退出信号处理
void handle_sigint(int sig)
{
    printf("\n[服务端] 收到退出信号，正在优雅关闭...\n");
    exit(0);
}

int main()
{
    signal(SIGINT, handle_sigint);

    int listen_fd = Socket_server_init(6666);
    if (listen_fd < 0) 
    {
        return 1;
    }
    printf("[服务端] 启动成功,epoll 监听端口 6666...\n");

    // 维护所有在线客户端，为了群发广播
    int client_fds[MAX_CLIENTS];
    char client_names[MAX_CLIENTS][32];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_fds[i] = -1;
        memset(client_names[i], 0, sizeof(client_names[i]));
    }

    // 1. 创建 epoll 实例
    int epfd = epoll_create(1);

    // 2. 把监听大门登记到 epoll
    struct epoll_event ev;
    ev.events = EPOLLIN; //表示可读
    ev.data.fd = listen_fd;//存门铃号
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    struct epoll_event events[1024];
    char buf[1024];

    while(1)
    {
        // 3. 躺平等待事件发生
        int n = epoll_wait(epfd, events, 1024, -1);
        if (n < 0)
        {
            perror("epoll_wait");
            break;
        }
        
        // 4. 遍历所有发生的事件
        for(int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;
            
            // ================= 情况 1：大门响了，新客户端连入 =================
            if (fd == listen_fd) 
            {
                struct sockaddr_in c_addr;
                socklen_t c_len = sizeof(c_addr);
                int c_fd = accept(listen_fd, (struct sockaddr*)&c_addr, &c_len);
                
                if (c_fd < 0) continue;
                
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
            // ================= 情况 2：老客户发消息或者断开 =================
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
                        // ------------------ 分支 1：起名包 ------------------
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
                        // ------------------ 分支 2：私聊包 (/) ------------------
                        else if (buf[0] == '/') 
                        {
                            char target_name[32];
                            char private_msg[1024];
                            
                            // 使用 sscanf 解析：/w 目标 内容
                            if (sscanf(buf, "/w %s %[^\n]", target_name, private_msg) == 2) 
                            {
                                int target_fd = -1;
                                
                                // 遍历找目标
                                for (int j = 0; j < MAX_CLIENTS; j++) 
                                {
                                    if (client_fds[j] != -1 && strlen(client_names[j]) > 0 && strcmp(client_names[j], target_name) == 0) 
                                    {
                                        target_fd = client_fds[j]; 
                                        break;
                                    }
                                }
                                
                                if (target_fd != -1) 
                                {
                                    char send_buf[2048];
                                    int send_len = snprintf(send_buf, sizeof(send_buf), "[私聊] %s: %s\n", client_names[slot], private_msg);
                                    send(target_fd, send_buf, send_len, 0);
                                    
                                    char reply_buf[128];
                                    int reply_len = snprintf(reply_buf, sizeof(reply_buf), "[系统] 已发送私聊给 %s\n", target_name);
                                    send(fd, reply_buf, reply_len, 0);
                                } 
                                else 
                                {
                                    char reply_buf[128];
                                    int reply_len = snprintf(reply_buf, sizeof(reply_buf), "[系统] 找不到用户 %s，或该用户不在线\n", target_name);
                                    send(fd, reply_buf, reply_len, 0);
                                }
                            }
                            else
                            {
                                // 格式不对的容错
                                char reply_buf[128];
                                int reply_len = snprintf(reply_buf, sizeof(reply_buf), "[系统] 格式错误，请使用: /w 目标名字 消息\n");
                                send(fd, reply_buf, reply_len, 0);
                            }
                        }
                        // ------------------ 分支 3：普通群聊包 ------------------
                        else // 👈 注意！这个 else 现在完美匹配到了 else if (buf[0] == '/')
                        {
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
                    } // 👈 这里才是 if (slot != -1) 的结束！
                }
                // ================= 情况 3：客户端断开连接 =================
                else 
                {
                    printf("客户端断开连接， fd = %d\n", fd);
                    
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);

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