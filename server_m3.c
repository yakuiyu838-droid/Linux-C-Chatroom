#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAX_CLIENTS FD_SETSIZE
#include "Socket_server_init.h"

void handle_sigint(int sig)
{
    printf("\n[服务端] 收到退出信号， 正在优雅关闭...\n");
    exit(0);
}

int main()
{
    signal(SIGINT, handle_sigint);
    int listen_fd = Socket_server_init(6666);
    if(listen_fd < 0)
    {
        return 1;
    } 
    printf("服务器启动成功，监听端口 6666...\n");
  
    int client_fds[MAX_CLIENTS];
    char client_names[MAX_CLIENTS][32];
    
    for(int i = 0; i < MAX_CLIENTS; i ++)     
    {
        client_fds[i] = -1;
        memset(client_names[i], 0, sizeof(client_names[i]));
    }

    int max_fd = listen_fd;  
    fd_set readfds;
    char buf[1024];

    while (1) 
    {
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);

        for(int i = 0; i < FD_SETSIZE; i++) 
        {
            if (client_fds[i] != -1) 
            {
            FD_SET(client_fds[i], &readfds);
            }
        }

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) break;
        
        // 1. 大门响了（新客户端接入）
        if(FD_ISSET(listen_fd, &readfds)) 
        {
            struct sockaddr_in c_addr;
            socklen_t c_len = sizeof(c_addr);
            int c_fd = accept(listen_fd, (struct sockaddr*)&c_addr, &c_len);

            if (c_fd < 0) continue;
            
            printf("成功连入，新 fd = %d\n", c_fd);
         
            for(int i = 0; i < FD_SETSIZE; i++) 
            {
                if(client_fds[i] == -1) 
                {
                    client_fds[i] = c_fd;
                    memset(client_names[i], 0, sizeof(client_names[i]));
                    break;
                }
            }   
            
            if(c_fd > max_fd) max_fd = c_fd;
        }

        // 2. 老客户发消息
        for (int i = 0; i < FD_SETSIZE; i++)
        {
            int fd = client_fds[i];
            
            if (fd == -1 || !FD_ISSET(fd, &readfds)) continue;
            
            int recv_len = recv(fd, buf, sizeof(buf) - 1, 0);
            
            if (recv_len > 0)
            {
                buf[recv_len] = '\0';
                printf("收到 fd = %d 的消息：%s\n", fd, buf);
                
                // 判断是不是起名（注意这里加了空格：NICK ）
                if(strncmp(buf, "NICK ", 5) == 0)
                {
                    strncpy(client_names[i], buf + 5, sizeof(client_names[i]) - 1);
                    client_names[i][strcspn(client_names[i], "\n")] = '\0';
                    printf("登记昵称：%s\n", client_names[i]);

                    // 广播提示上线
                    char sys_buf[128];
                    int sys_len = snprintf(sys_buf, sizeof(sys_buf), "[系统] %s 进入了聊天室\n", client_names[i]);
                 
                    for (int j = 0; j < FD_SETSIZE; j++)
                    {
                        if(client_fds[j] != -1 && client_fds[j] != fd )
                        {
                            send(client_fds[j], sys_buf, sys_len, 0);
                        }    
                    }
                }
                else
                {
                    char send_buf[2048];
                    int send_len = snprintf(send_buf, sizeof(send_buf), "%s: %s", client_names[i], buf);
                    for(int j = 0; j < FD_SETSIZE; j++ )
                    {
                        if(client_fds[j] != -1 && client_fds[j] != fd)
                        {
                            send(client_fds[j], send_buf, send_len, 0);
                        }
                    }
                }
            }
            else // 这里对应 recv_len <= 0
            {
                printf("客户端 %s 断开\n", client_names[i]);

                // 广播离开，只有起过名字的才会广播
                if(strlen(client_names[i]) > 0)
                {
                    char sys_buf[128];
                    int sys_len = snprintf(sys_buf, sizeof(sys_buf), "[系统] %s 离开了聊天室\n", client_names[i]);
                    for (int j = 0; j < FD_SETSIZE; j++)
                    {
                        if (client_fds[j] != -1 && client_fds[j] != fd)
                        {
                            send(client_fds[j], sys_buf, sys_len, 0);
                        }
                    }
                }
                
                // 清理现场
                close(fd);
                client_fds[i] = -1;
                memset(client_names[i], 0, sizeof(client_names[i]));

                // ⚠️ max_fd 的重算必须在这个断开连接的 else 里面！
                if (fd == max_fd) 
                {
                    max_fd = listen_fd;
                    for (int j = FD_SETSIZE - 1; j >= 0; j--) 
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
    } // 👈 注意：这是 while(1) 的结束括号

    close(listen_fd);
    return 0;
}