#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS FD_SETSIZE
#include "Socket_server_init.h"

int main()
{
    int listen_fd = Socket_server_init(6666);
    if(listen_fd < 0) 
    {
        return 1;
    }
    printf("服务器启动成功，监听端口 6666...\n");
  
    int client_fds[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        client_fds[i] = -1; // 全部设置为空座位
    }
    
    int max_fd = listen_fd;  
    fd_set readfds;
    char buf[1024];

    while (1) 
    {
        FD_ZERO(&readfds);                 // 擦黑板
        FD_SET(listen_fd, &readfds);       // 贴大门门铃

        for(int i = 0; i < FD_SETSIZE; i++) // 贴老顾客门铃
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
        
        // 1. 如果大门响了
        if(FD_ISSET(listen_fd, &readfds)) 
        {
            struct sockaddr_in c_addr;
            socklen_t c_len = sizeof(c_addr);
            int c_fd = accept(listen_fd, (struct sockaddr*)&c_addr, &c_len);

            if (c_fd < 0) 
            {
                perror("accept");
                continue; 
            }
            
            printf("成功连入，新 fd = %d\n", c_fd);
         
            for(int i = 0; i < FD_SETSIZE; i++) 
            {
                if(client_fds[i] == -1) 
                {
                    client_fds[i] = c_fd;
                    break;
                }
            }   
            
            if(c_fd > max_fd) 
            {
                max_fd = c_fd;
            }
        }

        // 2. 遍历所有老客户
        for (int i = 0; i < FD_SETSIZE; i++)
        {
            int fd = client_fds[i];
            
            // 过滤空座 和 没动静的客人
            if (fd == -1 || !FD_ISSET(fd, &readfds)) 
            {
                continue;
            }
            
            // 接收数据
            int recv_len = recv(fd, buf, sizeof(buf) - 1, 0);
            
            if (recv_len > 0) 
            {
                buf[recv_len] = '\0';
                printf("收到 fd=%d 的消息: %s\n", fd, buf);
                
                // ================== 带名字的广播 ==================
                char send_buf[2048]; 
                // 拼接： "用户[fd]: [消息内容]"
                int send_len = snprintf(send_buf, sizeof(send_buf), "用户%d: %s", fd, buf);

                // 群聊广播（发给除了自己以外的其他人）
                for (int j = 0; j < FD_SETSIZE; j++) 
                {
                    if (client_fds[j] != -1 && client_fds[j] != fd) 
                    {
                        send(client_fds[j], send_buf, send_len, 0);
                    }
                }
            } 
            else 
            {
                printf("客户端 fd=%d 断开\n", fd);
                close(fd);              // 1. 关掉套接字
                client_fds[i] = -1;     // 2. 空出座位
                
                // 3. 如果断开的是最大的 fd，必须重新找最大值
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
    } // 👈 while 循环结束

    close(listen_fd);
    return 0;
}