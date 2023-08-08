#pragma once

#include <stdint.h>
#include <stdio.h>
#include <poll.h>
#include <stdbool.h>
#include "conn.h"

#define MAX_CONNECTIONS 1024
#define MAX_ADDR_STRING_SIZE 256
typedef struct Server Server;

struct Server{
    int fd;
    Conn* connections[MAX_CONNECTIONS];
    size_t num_connections;
    struct pollfd poll_args[MAX_CONNECTIONS + 1];

    char addr_str[MAX_ADDR_STRING_SIZE];
    uint16_t port;
};

bool rcpy_server_init(Server *server, const char *addr_string, uint16_t port);
void rcpy_server_quit(Server *server);
bool rcpy_server_start_loop(Server *server);
