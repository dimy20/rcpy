#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "io.h"
#include "socket.h"

#define ERR_LOG(s) fprintf(stderr, "Error: %s\n", s)

#define PORT 8080
#define HOST "127.0.0.1"
#define BACKLOG 64

#define MAX_MSG_SIZE 4096
#define HEADER_SIZE 4
#define MAX_CONNECTIONS 1024
#define ARR_LEN(arr) sizeof(arr) / sizeof(arr[0])

static int one_request(int client_socket);

typedef struct Conn Conn;
typedef struct Server Server;
typedef struct Buffer Buffer;

typedef enum{
    REQUEST,
    RESPONSE,
    END
}ConnState;

struct Buffer{
    char data[MAX_MSG_SIZE + HEADER_SIZE + 1];
    size_t start;
    size_t end;
};

struct Conn{
    int fd;
    char read_buffer[MAX_MSG_SIZE + HEADER_SIZE + 1];
    char write_buffer[MAX_MSG_SIZE + HEADER_SIZE];
    size_t read_buffer_size;
    size_t write_buffer_size;
    size_t write_buffer_sent;
    ConnState state;
    Buffer read_buf;
};

struct Server{
    int fd;
    Conn* connections[MAX_CONNECTIONS];
    size_t num_connections;
    struct pollfd poll_args[MAX_CONNECTIONS + 1];
};

bool rw_server_init(Server *server){
    if(!socket_init(&server->fd, SOCKET_NON_BLOCK)) return false;

    socket_make_nonblock(server->fd);
    memset(server->connections, 0, sizeof(Conn *));
    return true;
}

Server server;

void rw_server_prepare_poll_args(Server * server){
    memset(server->poll_args, 0, sizeof(server->poll_args));

    server->poll_args[0].fd = server->fd;
    server->poll_args[0].events = POLLIN;
    server->poll_args[0].revents = 0;

    for(size_t i = 0; i < ARR_LEN(server->poll_args); i++){
        Conn* conn = server->connections[i];
        if(conn != NULL){
            server->poll_args[i].fd = conn->fd;
            server->poll_args[i].events = conn->state == REQUEST ? POLLIN : POLLOUT;
            server->poll_args[i].events |= POLLERR;
        }
    };
};

void rw_conn_state_request(Conn * conn){
    do{
        size_t cap = conn->read_buf.end - conn->read_buf.start;
        ssize_t ret = read(conn->fd, conn->read_buf.data + conn->read_buf.start, cap);
    }while(ret < 0 && errno == EINTR);


    conn->read_buf.start += ret;

    m_read_buffer.cut_message(); //rw::buffer_cut_message(&conn->read_buff);
    //rw_buffer_cut_message(&conn-
    //:>read_buf);
};

void rw_conn_process_io(Conn *conn){
    if(conn->state == REQUEST){
        rw_conn_state_request(conn):
    }
};

void rw_server_process_connections(Server *server){
    for(int i = 1; i < ARR_LEN(server->poll_args); i++){
        if(server->poll_args[i].revents){
            int fd = server->poll_args[i].fd;
            Conn *conn = server->connections[fd];

            rw_conn_process_io(conn);

            if(conn->state == END){
                server->connections[conn->fd] = NULL;
                close(conn->fd);
                free(conn);
            };
        };
    };
};

int main(){
    if(!rw_server_init(&server)){
        exit(1);
    }
    if(!socket_listen(server.fd, HOST, PORT)){
        exit(1);
    }

    while(1){
        rw_server_prepare_poll_args(&server);

        int ret;
        ret = poll(server.poll_args, ARR_LEN(server.poll_args), 0);

        if(ret < 0){
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(1);
        };


        rw_server_process_connections(&server);


    };

    return 0;
}

/*
static int one_request(int client_socket){
    char buffer[HEADER_SIZE + MAX_MSG_SIZE + 1];
    memset(buffer, 0, HEADER_SIZE + MAX_MSG_SIZE + 1);
    errno = 0;

    int err = read_all(client_socket, buffer, HEADER_SIZE);

    if(err){
        if(errno == 0){
            fprintf(stderr, "EOF\n");
        }else{
            ERR_LOG(strerror(errno));
        }
        return err;
    }

    uint32_t len;
    memcpy(&len, buffer, 4); // assuming little edian; bad

    if(len > MAX_MSG_SIZE){
        ERR_LOG("Message too long");
        return -1;
    }

    err = read_all(client_socket, buffer + HEADER_SIZE, len);
    if(err){
        ERR_LOG(strerror(err));
        return err;
    }

    buffer[HEADER_SIZE + MAX_MSG_SIZE] = '\0';
    printf("Client says: %s\n", buffer + HEADER_SIZE);

    const char * reply = "Bitch";
    size_t reply_len = strlen(reply);
    char response_buff[HEADER_SIZE + MAX_MSG_SIZE + 1];

    memcpy(response_buff, &reply_len, 4);
    memcpy(response_buff + HEADER_SIZE, reply, reply_len);

    write_all(client_socket, response_buff, HEADER_SIZE + reply_len);
    return 0;
};
*/
