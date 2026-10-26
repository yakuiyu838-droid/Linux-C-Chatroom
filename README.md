# 基于 Linux epoll 的高并发聊天室 (Linux-C-Chatroom)

## 📖 项目简介
本项目是一个基于纯 C 语言开发的 TCP 多人在线聊天室。服务端采用 Linux 独有的 `epoll` I/O 多路复用技术实现单线程高并发；客户端采用多线程 (`pthread`) 实现全双工通信。

## ✨ 核心功能
- **全局群聊**：消息实时广播给所有在线用户。
- **定向私聊**：通过 `/w 目标昵称 消息` 指令实现一对一私密通信。
- **状态管理**：支持登录起名，实时广播上下线通知。
- **协议设计**：自定义基于 `\n` 为边界的应用层协议，解决 TCP 粘包/半包问题。
- **优雅停机**：服务端捕获 `SIGINT` 信号，实现按 `Ctrl+C` 时的资源安全释放。

## 🛠️ 技术栈
- **语言**：C (C11)
- **环境**：Linux / WSL (Ubuntu)
- **核心技术**：Socket 编程、`epoll` 事件驱动、多线程、TCP/IP 协议栈

## 🚀 编译与运行
1. 编译服务端：
   `gcc src/server_epoll_whisper.c src/Socket_server_init.c -o server`
2. 编译客户端：
   `gcc src/client_m3.c src/Socket_client_init.c -o client -pthread`
3. 启动服务端：
   `./server`
4. 启动客户端（可多开）：
   `./client`