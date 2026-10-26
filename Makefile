CC      = gcc
CFLAGS  = -Wall -Wextra -g

all: server client

# 服务端用了 pthread，链接时需要 -pthread
server: my_echo_server.c
	$(CC) $(CFLAGS) -pthread -o server $^

client: my_echo_client.c
	$(CC) $(CFLAGS) -o client $^

clean:
	rm -f server client

.PHONY: all clean
