#include <assert.h>
#include <netinet/in.h>
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

#include <unordered_map>
#include <vector>
#include <string>

#define CMD_IS(s, cmd) strncmp(s, cmd, strlen(cmd)) == 0

#define TODO(s) do{ \
    fprintf(stderr, "TODO: %s\n, at %s:%d", s, __FILE__, __LINE__); \
    exit(1); \
}while(0);

#define CMD_GET "get"
#define CMD_SET "set"
#define CMD_DEL "del"

#define MAX_CMD_NUM_STRINGS 8 // Maximum number of argument strings for command
#define MAX_ARG_STRING_SIZE 128 // Maximum length of an argument string

static void conn_cut_message(Conn *conn, size_t msg_len);
static bool conn_do_request(Conn *conn, size_t msg_len);
static bool parse_request_message(Conn &conn, std::vector<std::string>& out_cmd, size_t msg_len);
static Response do_get_cmd(const std::vector<std::string>& cmd);
static Response do_set_cmd(const std::vector<std::string>& cmd);
static Response do_del_cmd(const std::vector<std::string>& cmd);

static std::unordered_map<std::string, std::string> map;
int count = 0;

static void on_write_cb(uv_write_t *req, int status){
    assert(req != NULL);
    if(status < 0){
        UV_ERR_LOG("on_write_cb", status);
        free(req);
        return;
    }else{
        free(req);
        //stupid thing
        printf("count = %d\n", ++count);
        if(count >= 350){
            uv_stop(uv_default_loop());
        }
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

    int ret;
    if((ret = uv_fileno((const uv_handle_t*)stream, &conn->fd) < 0)){
        UV_ERR_LOG("uv_fileno", ret);
        return false;
    }

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

static void conn_prepare_write_buffer(Conn *conn, const Response& response){
    uint32_t total_bytes = htonl(4 + response.response_string.size());
    uint32_t status = htonl(response.status);

    memcpy(conn->write_buffer.data, &total_bytes, 4);
    memcpy(conn->write_buffer.data + 4, &status, 4);

    memcpy(conn->write_buffer.data + 8,
           response.response_string.c_str(),
           response.response_string.size());
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

enum ResponseStatus{
    OK = 0,
    ERR = 1,
    NO_VALUE = 2,
};

/*
 *  N = number of strings
 *  [N:len1:string1:len2:string2:len3:string3....:lenN:stringN]
 */

static bool parse_request_message(const char* buffer, size_t msg_len, std::vector<std::string>& cmd){
    assert(msg_len < MAX_MSG_SIZE);

    if(msg_len < 4){
        /* Make sure we have at least 4 bytes to read the number of strings.
         * num_strings : uint32_t.
         */
        return false;
    }

    /*Get the number of strings that make up the command.*/
    uint32_t num_strings;
    memcpy(&num_strings, buffer, 4);
    num_strings = ntohl(num_strings);

    if(num_strings > MAX_CMD_NUM_STRINGS){
        return false;
    }

    /*Start at first string length and read all strings.*/
    size_t pos = 4;
    for(size_t i = 0; i < num_strings; i++){
        if(pos + 4 > msg_len){
            return false;
        }

        uint32_t string_len;
        memcpy(&string_len, buffer + pos, 4);
        string_len = ntohl(string_len);

        if(pos + 4 + string_len > msg_len){
            return false;
        }

        std::string s(buffer + pos + 4, string_len);
        cmd.push_back(std::move(s));

        pos += (4 + string_len);
    };

    if(pos < msg_len){
        return false;
    }

    return true;
};


static Response do_get_cmd(const std::vector<std::string>& cmd){
    assert(cmd.size() == 2);

    //assume failure
    Response res;
    res.status = ResponseStatus::NO_VALUE;
    res.response_string[0] = '\0';

    const std::string& key = cmd[1];

    if(map.find(key) == map.end()){
        return res;
    }

    auto& value = map[key];

    res.status = ResponseStatus::OK;
    res.response_string = value;

    return res;
};

static Response do_del_cmd(const std::vector<std::string>& cmd){
    assert(cmd.size() == 2);

    auto& key = cmd[1];
    map.erase(key);

    Response res;
    res.status = ResponseStatus::OK;

    return res;
};

static Response do_set_cmd(const std::vector<std::string>& cmd){
    assert(cmd.size() == 3);

    auto& key = cmd[1];
    auto& value = cmd[2];
    map[key] = value;

    Response res;
    res.status = ResponseStatus::OK;

    return res;
};

static bool do_request(const char *buffer, size_t msg_len, Response *response){
    std::vector<std::string> cmd;

    if(!parse_request_message(buffer, msg_len, cmd)){
        ERR_LOG("Bad request");
        return false;
    };

    for(auto& c : cmd){
        printf("%s\n", c.c_str());
    }
    assert(!cmd.empty());

    if(cmd.size() == 3 && cmd[0] == CMD_SET){
        *response = do_set_cmd(cmd);
    }else if(cmd.size() == 2 && cmd[0] == CMD_GET){
        *response = do_get_cmd(cmd);
    }else if(cmd.size() == 2  && cmd[0] == CMD_DEL){
        *response = do_del_cmd(cmd);
    }else{
        Response res;
        res.status = ResponseStatus::ERR;
        res.response_string = "Uknown command";
        *response = res;
    }
    return true;
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

    Response response;
    if(!do_request(conn->read_buffer.data + 4, msg_len, &response)){
        conn->state = CONN_STATE_END;
        return false;
    }

    //Just for now
    exit(1);

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
