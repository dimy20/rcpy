#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include "conn.h"

#define MAX_CONNECTIONS 1024
#define MAX_ADDR_STRING_SIZE 256
typedef struct Server Server;

struct Server{
    int fd;
    Conn* connections[MAX_CONNECTIONS];
    struct epoll_event events[MAX_CONNECTIONS];

    size_t num_connections;
    char addr_str[MAX_ADDR_STRING_SIZE];
    uint16_t port;
    int efd;
    bool alive;
};

bool rcpy_server_init(Server *server, const char *addr_string, uint16_t port);
void rcpy_server_quit(Server *server);
bool rcpy_server_start_loop(Server *server);
