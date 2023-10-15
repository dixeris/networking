all: chat_server chat_client

chat_server: socketheader.h chat_server.c Makefile
	gcc chat_server.c -g -o $@ -pthread

chat_client: socketheader.h chat_client.c
	gcc chat_client.c -g -o $@

