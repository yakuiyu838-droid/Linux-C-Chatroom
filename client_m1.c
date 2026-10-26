#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

 "#include "Socket_client_init.h""


#define SERVER_IP "127.0.0.1"
#define PORT 6666
#define BUF_SIZE 1024

int main() 
{
    char buf[BUF_SIZE];
    ssize_t recv_len, send_len;
    size_t len;

    // ================== 1. 调用封装好的连接函数 ==================
    printf("[客户端] 正在连接服务端 %s:%d...\n", SERVER_IP, PORT);
    int client_fd = Socket_client_init(SERVER_IP, PORT);
    if (client_fd < 0) 
    {
        printf("[客户端] 连接失败，程序退出\n");
        return 1;
    }
    printf("[客户端] 已成功连接到服务端！\n");
    printf("[客户端] 输入消息发送给服务端（输入 exit 或 quit 退出）\n");

    // ================== 2. 循环：读键盘 -> 发送 -> 收回复 ==================
    while (1) 
    {
        printf("> ");
        fflush(stdout); // 强制刷新提示符

        if (fgets(buf, BUF_SIZE, stdin) == NULL) break; // 输入结束或出错

        // 清洗数据：去掉末尾的 \n 和 \r
        len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        {
            buf[--len] = '\0';
        }

        if (len == 0) continue; // 空行不发送
        if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) break;

        // 发送消息
        send_len = send(client_fd, buf, len, 0);
        if (send_len < 0)
        {
            perror("send");
            break;
        }

        // 接收服务端回显
        recv_len = recv(client_fd, buf, BUF_SIZE - 1, 0);
        if (recv_len > 0) 
        {
            buf[recv_len] = '\0';
            printf("[客户端] 收到服务端回复: %s\n", buf);
        } 
        else if (recv_len == 0) 
        {
            printf("[客户端] 服务端已关闭连接\n");
            break;
        } 
        else 
        {
            perror("recv");
            break;
        }
    }

    // ================== 3. 关闭套接字 ==================
    close(client_fd);
    printf("[客户端] 程序退出\n");
    return 0;
}