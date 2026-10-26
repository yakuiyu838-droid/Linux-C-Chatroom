#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Socket_client_init.h"

#define SERVER_IP "127.0.0.1"
#define PORT 6666
#define BUF_SIZE 1024

void *receive_thread(void *arg) 
{
    int client_fd = *(int *)arg;
    char buf[BUF_SIZE];
    int recv_len;

    while (1) 
    {
        recv_len = recv(client_fd, buf, BUF_SIZE - 1, 0);
        if (recv_len > 0) 
        {
            buf[recv_len] = '\0';
            // 客户端切包逻辑完全没变
            char *msg = strtok(buf, "\n");
            while (msg != NULL) 
            {
                printf("\n[新消息] %s\n> ", msg);
                fflush(stdout);
                msg = strtok(NULL, "\n");
            }
        } 
        else if (recv_len == 0) 
        {
            printf("\n[客户端] 服务端已断开连接\n");
            exit(0);
        } 
        else 
        {
            perror("recv");
            exit(1);
        }
    }
    return NULL;
}

int main() 
{
    char buf[BUF_SIZE];
    int send_len;

    int client_fd = Socket_client_init(SERVER_IP, PORT);
    if (client_fd < 0) return 1;
    printf("[客户端] 已成功连接到服务端！\n");
   
    //让用户输入名字并发送  
    char name[32];
    printf("请输入你的昵称：");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';//去掉回车

    char nick_buf[64];
    int n = snprintf(nick_buf, sizeof(nick_buf), "NICK %s\n", name);
    send(client_fd, nick_buf, n, 0);
  

    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_thread, &client_fd) != 0) 
    {
        perror("pthread_create");
        return 1;
    }
    pthread_detach(tid);

    while (1) 
    {
        printf("> ");
        fflush(stdout);
        if (fgets(buf, BUF_SIZE, stdin) == NULL) break;

        if (strncmp(buf, "exit", 4) == 0 || strncmp(buf, "quit", 4) == 0) 
        {
            break;
        }

        send_len = send(client_fd, buf, strlen(buf), 0);
        if (send_len < 0) 
        {
            perror("send");
            break;
        }
    }

    close(client_fd);
    return 0;
}