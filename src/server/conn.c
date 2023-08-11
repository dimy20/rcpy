#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <uv.h>
#include "socket.h"
#include "conn.h"
#include "error.h"

static void conn_cut_message(Conn *conn, size_t msg_len);
int count = 0;

static void on_write_cb(uv_write_t *req, int status){
    assert(req != NULL);
    if(status < 0){
        UV_ERR_LOG("on_write_cb", status);
        free(req);
        return;
    }else{
        //stupid thing
        printf("count = %d\n", ++count);
        if(count >= 350){
            uv_stop(uv_default_loop());
        }
        free(req);
    }
};

static void init_read_buffer(ReadBuf *read_buf){
    assert(read_buf != NULL);
    memset(read_buf->data, 0, sizeof(read_buf->data));
    read_buf->size = 0;
    read_buf->cap = sizeof(read_buf->data);
}

static void init_write_buffer(WriteBuf *write_buff){
    assert(write_buff != NULL);
    memset(write_buff->data, 0, sizeof(write_buff->data));
    write_buff->size = 0;
    write_buff->sent = 0;
};

bool conn_init(Conn *conn, uv_tcp_t *stream){
    assert(conn != NULL && stream != NULL);
    init_read_buffer(&conn->read_buffer);
    init_write_buffer(&conn->write_buffer);
    conn->state = CONN_STATE_REQUEST;
    conn->stream = stream;
    stream->data = conn;
    return true;
};

/* Removes a message of size msg_len from the connections read buffer.
 * This function is meant to be called after the message has been
 * processed.
 * */
static void conn_cut_message(Conn *conn, size_t msg_len){
    assert(conn != NULL && conn->read_buffer.size >= (HEADER_SIZE + msg_len));
    size_t processed_segment_size = (HEADER_SIZE + msg_len);
    size_t remaining_size = conn->read_buffer.size - processed_segment_size;

    if(remaining_size > 0){
        memmove(conn->read_buffer.data,
                conn->read_buffer.data + processed_segment_size,
                remaining_size);
    };
    conn->read_buffer.size = remaining_size;
};

static void conn_make_echo_response(Conn *conn, const char *msg, uint32_t msg_len){
    assert(conn != NULL);
    uint32_t len = htonl(msg_len);
    memcpy(conn->write_buffer.data, &len, 4);
    memcpy(conn->write_buffer.data + 4, msg, msg_len);
    conn->write_buffer.size = 4 + msg_len;
};

static uv_write_t * conn_make_write_request(Conn* conn, uv_buf_t * uv_write_buffer){
    assert(conn != NULL);
    assert(uv_write_buffer != NULL);

    uv_write_t *req = (uv_write_t *)RCPY_MALLOC(sizeof(uv_write_t));
    /* Buffer sent to write request points to the connection's write buffer.
     * */
    *uv_write_buffer = uv_buf_init(conn->write_buffer.data, conn->write_buffer.size);
    req->data = conn;

    return req;
};

bool conn_process_request_message(Conn *conn){
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

    //Begin processing
    char msg[MAX_MSG_SIZE];
    memset(msg, 0, sizeof(msg));
    memcpy(msg, conn->read_buffer.data + HEADER_SIZE, msg_len);
    //End processing
    // message processing done, remove from buffer
    conn_cut_message(conn, msg_len);

    conn_make_echo_response(conn, msg, msg_len);
    uv_buf_t uv_write_buffer;
    uv_write_t * write_request = conn_make_write_request(conn, &uv_write_buffer);

    int ret;
    if((ret = uv_write(write_request,
                      (uv_stream_t *)conn->stream,
                       &uv_write_buffer,
                       1,
                       on_write_cb)) < 0){
        UV_ERR_LOG("uv_write", ret);
        free(write_request);
        return false;
    }
    
    return true;
    /*
     * True return signals that we sucessfully processed a message and that 
     * we can try to process the next one, im asuming that it always succeeds to respond,
     * simply because responding hasnt been implemented yet, the only source of failure
     * right now is incomplete data on buffer, which is signaled up and eventually the
     * upper layers hits eagain.
     * */
};

void conn_read_incoming_data(Conn *conn, size_t nread, const uv_buf_t *uv_buf){
    assert(conn != NULL);
    assert(uv_buf != NULL);

    size_t read_offset = 0;
    while(nread > 0){

        size_t available_space = conn->read_buffer.cap - conn->read_buffer.size;
        size_t read_amount = 0;
        if(nread > available_space){
            read_amount = available_space;
        }else{
            read_amount = nread;
        }

        memcpy(conn->read_buffer.data + conn->read_buffer.size, 
               uv_buf->base + read_offset,
               read_amount);

        nread -= read_amount;
        conn->read_buffer.size += read_amount;
        read_offset += read_amount;

        //printf("av bytes: %ld\n", av_bytes);
        assert(conn->read_buffer.size <= conn->read_buffer.cap
                && "Max connection buffer exceeded");

        // process as many message as there are available on the buffer
        while(conn_process_request_message(conn));
    }
};
