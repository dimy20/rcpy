#pragma once

typedef struct Conn Conn;
typedef struct Buffer Buffer;
#include <stdio.h>

#define MAX_MSG_SIZE 4096
#define HEADER_SIZE 4

typedef enum{
    CONN_STATE_REQUEST,
    CONN_STATE_RESPONSE,
    CONN_STATE_END,
}ConnState;

typedef struct{
    char data[MAX_MSG_SIZE + HEADER_SIZE + 1];
    size_t size;
    size_t cap;
}ReadBuf;

struct Conn{
    int fd;
    ReadBuf read_buffer;
    char write_buffer[MAX_MSG_SIZE + HEADER_SIZE];
    size_t write_buffer_size;
    size_t write_buffer_sent;
    ConnState state;
};

bool conn_init(Conn *conn, int fd);
void conn_io_read_many(Conn *conn);

