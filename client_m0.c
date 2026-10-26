#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8888
#define BUF_SIZE 1024

int main() 
{
    int client_fd;
    struct sockaddr_in server_addr;
    char buf[BUF_SIZE];
    ssize_t recv_len, send_len;
    size_t len;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) { perror("socket"); return 1; }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        printf("非法地址: %s\n", SERVER_IP); close(client_fd); return 1;
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); close(client_fd); return 1;
    }
    printf("[客户端] 已连接到服务端 %s:%d\n", SERVER_IP, PORT);
    printf("[客户端] 输入消息（输入 exit 或 quit 退出）\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(buf, BUF_SIZE, stdin) == NULL) break;

        len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }
        if (len == 0) continue;
        if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) break;

        send_len = send(client_fd, buf, len, 0);
        if (send_len < 0) { perror("send"); break; }

        recv_len = recv(client_fd, buf, BUF_SIZE - 1, 0);
        if (recv_len > 0) {
            buf[recv_len] = '\0';
            printf("[客户端] 收到回复: %s\n", buf);
        } 
        else if (recv_len == 0) {
            printf("[客户端] 服务端已关闭\n"); break;
        } 
        else { perror("recv"); break; }
    }

    close(client_fd);
    printf("[客户端] 程序退出\n");
    return 0;
}