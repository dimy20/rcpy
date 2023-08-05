#pragma once
#include <stdint.h>
#include <arpa/inet.h>
#include <stdbool.h>

enum SockFlag{
    SOCKET_NON_BLOCK = 1,
};

bool socket_init(int *fd, uint32_t flags);
bool socket_make_nonblock(int fd);
bool socket_listen(int fd, const char *host, uint16_t port);
bool socket_addr_init(struct sockaddr_in *addr, const char *host, uint16_t port);
