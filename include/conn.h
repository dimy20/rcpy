#pragma once

#include <sys/socket.h>
typedef struct Conn Conn;
typedef struct Buffer Buffer;
#include <stdio.h>
#include <uv.h>

#include <vector>
#include <string>

#define MAX_MSG_SIZE 32768 //32kb
#define HEADER_SIZE 4
#define MAX_CMDS 128
#define MAX_CMDS_STRING_SIZE 256

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

struct Response{
    uint32_t status;
    std::string response_string;
};

struct Conn{
    ReadBuf read_buffer;
    WriteBuf write_buffer;
    ConnState state;
    uv_tcp_t *stream;
    uv_os_fd_t fd;
};

bool conn_init(Conn *conn, uv_tcp_t *stream);
bool conn_process_request_message(Conn *conn);
void conn_read_incoming_data(Conn *conn, size_t nread, const uv_buf_t *uv_buf);
