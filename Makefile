# 编译器与编译选项
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11

# 目标文件
SERVER_TARGET = server
CLIENT_TARGET = client

# 源文件路径
SRC_DIR = chatroom_final
SERVER_SRCS = $(SRC_DIR)/server_epoll_whisper.c $(SRC_DIR)/Socket_server_init.c
CLIENT_SRCS = $(SRC_DIR)/client_m3.c $(SRC_DIR)/Socket_client_init.c

# 默认目标：编译服务端和客户端
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# 编译服务端
$(SERVER_TARGET): $(SERVER_SRCS)
	$(CC) $(CFLAGS) $^ -o $@

# 编译客户端（注意：只有客户端需要 -pthread）
$(CLIENT_TARGET): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ -pthread

# 清理编译产生的可执行文件
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

# 声明伪目标，防止同名文件冲突
.PHONY: all clean