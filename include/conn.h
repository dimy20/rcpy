#pragma once

typedef struct Conn Conn;
typedef struct Buffer Buffer;
#include <stdio.h>

#define MAX_MSG_SIZE 4096
#define HEADER_SIZE 4

typedef enum{
    CONN_STATE_REQUEST,
    CONN_STATE_RESPONSE,
    CONN_STATE_END
}ConnState;

struct Buffer{
    char data[MAX_MSG_SIZE + HEADER_SIZE + 1];
    size_t start;
    size_t end;
};

struct Conn{
    int fd;
    Buffer read_buffer;
    char write_buffer[MAX_MSG_SIZE + HEADER_SIZE];
    size_t write_buffer_size;
    size_t write_buffer_sent;
    ConnState state;
};

bool rcpy_conn_init(Conn *conn, int fd);
void rcpy_buffer_init(Buffer *buffer);
