#include <assert.h>
#include <string.h>
#include "socket.h"
#include "conn.h"

void rcpy_buffer_init(Buffer *buffer){
    assert(buffer != NULL);
    memset(buffer->data, 0, sizeof(buffer->data));
    buffer->start = 0;
    buffer->end = sizeof(buffer->data) / sizeof(buffer->data[0]);
};

bool rcpy_conn_init(Conn *conn, int fd){
    assert(conn != NULL);
    conn->fd = fd;
    rcpy_buffer_init(&conn->read_buffer);
    memset(conn->write_buffer, 0, sizeof(conn->write_buffer));
    conn->write_buffer_sent = 0;
    conn->write_buffer_size = 0;
    conn->state = CONN_STATE_REQUEST;

    return socket_make_nonblock(fd);
};
