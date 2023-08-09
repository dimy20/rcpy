#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "socket.h"
#include "conn.h"
#include "error.h"

static bool conn_process_request_message(Conn *conn);
static void conn_cut_message(Conn *conn, size_t msg_len);
static bool conn_io_read(Conn *conn);

static void init_read_buffer(ReadBuf *read_buf){
    memset(read_buf->data, 0, sizeof(read_buf->data));
    read_buf->size = 0;
    read_buf->cap = sizeof(read_buf->data);
}

bool conn_init(Conn *conn, int fd){
    assert(conn != NULL);
    assert(fd > 0);

    conn->fd = fd;
    init_read_buffer(&conn->read_buffer);

    memset(conn->write_buffer, 0, sizeof(conn->write_buffer));
    conn->write_buffer_sent = 0;
    conn->write_buffer_size = 0;
    conn->state = CONN_STATE_REQUEST;

    return socket_make_nonblock(fd);
};

/*
 * conn_io_read should be called when there is a read event from epoll.
 * It will read and process as many messages as possible until it hits
 * eagain, eof or and error occurs;
 *
 * */
void conn_io_read_many(Conn *conn){
    while(conn_io_read(conn));
};

static bool conn_io_read(Conn *conn){
    assert(conn != NULL);
    // Try to fill the buffer
    ssize_t ret;
    // [len<------->len<-------->len<-------->]
    //        ^        ^   ^                  ^
    size_t space_left = conn->read_buffer.cap - conn->read_buffer.size;
    do{
        ret = read(conn->fd, conn->read_buffer.data + conn->read_buffer.size, space_left);
    }while(ret < 0 && errno == EINTR);

    /* No data available at the moment, stop and wait for more data.*/
    if(ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
        conn->state = CONN_STATE_REQUEST;
        return false;
    }

    /* Some error happened, close the connection */
    if(ret < 0){
        perror("read");
        conn->state = CONN_STATE_END;
        return false;
    }

    if(ret == 0){
        LOG("EOF");
        conn->state = CONN_STATE_END;
        return false;
    }

    assert(conn->read_buffer.size + ret <= conn->read_buffer.cap);

    conn->read_buffer.size += (size_t)ret;
    // process as many messages as possible
    while(conn_process_request_message(conn));

    /* 
     *  At this point conn_process_request_message has failed, 
     *  for one of the two following reasons:
     *  1) Not enought data on buffer to read message
     *  2) Failed to process message (Generate response) due to eagain on out buffer.
     *
     *  if we return true we are signaling the upper layer that it should try to put
     *  more data on the buffer, this should only happen if the exit reason was 1.
     *
     *  if reason was 2 we interrupt reading and suscribe for writing event.
     *
     *  Right now im always returning true because im assuming writing never fails
     *  (server response hasnt been implemented yet), but this should be handled correctly
     *  as soon as responses are introduced.
     * */
    return true;
}

/* Removes a message of size msg_len from the connections read buffer.
 * This function is meant to be called after the message has been
 * processed.
 * */
static void conn_cut_message(Conn *conn, size_t msg_len){
    assert(conn->read_buffer.size >= (HEADER_SIZE + msg_len));
    size_t processed_segment_size = (HEADER_SIZE + msg_len);
    size_t remaining_size = conn->read_buffer.size - processed_segment_size;

    if(remaining_size > 0){
        memmove(conn->read_buffer.data,
                conn->read_buffer.data + processed_segment_size,
                remaining_size);
    };
    conn->read_buffer.size = remaining_size;
};

static bool conn_process_request_message(Conn *conn){
    //Not enough data to read message length.
    if(conn->read_buffer.size < HEADER_SIZE){
        return false;
    };

    uint32_t msg_len;
    memcpy(&msg_len, conn->read_buffer.data, HEADER_SIZE);
    // network byte order to host.
    msg_len = ntohl(msg_len);

    if(msg_len > MAX_MSG_SIZE){
        LOG("message length exceeds maximum message length.");
        conn->state = CONN_STATE_END;
        return false;
    };

    //Not enough data has arrive to read full message
    if(conn->read_buffer.size < HEADER_SIZE + msg_len){
        return false;
    }

    char msg[MAX_MSG_SIZE];
    memset(msg, 0, sizeof(msg));
    memcpy(msg, conn->read_buffer.data + HEADER_SIZE, msg_len);

    printf("Client says: %s\n", msg);
    // message processing done, remove from buffer
    conn_cut_message(conn, msg_len);
    conn->state = CONN_STATE_REQUEST;

    /*
     * True return signals that we sucessfully processed a message and that 
     * we can try to process the next one, im asuming that it always succeeds to respond,
     * simply because responding hasnt been implemented yet, the only source of failure
     * right now is incomplete data on buffer, which is signaled up and eventually the
     * upper layers hits eagain.
     * */
    return true;
};
