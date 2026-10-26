#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Socket_server_init.h"

// 封装服务端初始化：创建套接字、绑定端口、开始监听
// 参数 port: 你要监听的端口号（比如 8888）
// 返回值: 成功返回监听套接字 fd，失败返回 -1
int Socket_server_init(int port) 
{
    int listen_fd;
    struct sockaddr_in server_addr;
    int opt = 1; // 用于 setsockopt 端口复用

    // 1. 创建套接字
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return -1;
    }

    // 2. 设置端口复用 (极其重要，防止重启时报 Address already in use)
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return -1;
    }

    // 3. 填充服务端地址结构体
    memset(&server_addr, 0, sizeof(server_addr)); // 清空防垃圾
    server_addr.sin_family = AF_INET;             // IPv4 协议
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网卡
    server_addr.sin_port = htons(port);           // 端口转网络字节序

    // 4. 绑定套接字
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd); // 绑失败要关掉前面创建的 fd
        return -1;
    }

    // 5. 开始监听
    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    return listen_fd; // 成功，返回监听套接字
}