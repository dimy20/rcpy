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

typedef struct{
    char data[MAX_MSG_SIZE + HEADER_SIZE + 1];
    size_t sent; // How much has been sent at a given point (after one or many write calls)
    size_t size; // The total amount of data the has to be sent.
}WriteBuf;

struct Conn{
    int fd;
    ReadBuf read_buffer;
    WriteBuf write_buffer;
    ConnState state;
};

bool conn_init(Conn *conn, int fd);
void conn_io_read_many(Conn *conn);
void conn_io_write(Conn *conn);

