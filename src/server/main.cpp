#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "io.h"
#include "socket.h"
#include <vector>

#define ERR_LOG(s) fprintf(stderr, "Error: %s\n", s)

#define PORT 8080
#define HOST "127.0.0.1"
#define BACKLOG 64

#define MAX_MSG_SIZE 4096
#define HEADER_SIZE 4

static int one_request(int client_socket);

struct Server{
    Server(){};
    bool init();

    int fd;
};

bool Server::init(){
    if(!net::socket_init(fd, net::SockFlag::NON_BLOCK)) return false;
    if(!net::socket_listen(fd, HOST, PORT)) return false;
    return true;
};

int main(){
    Server server;
    if(!server.init()) return 1;

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = 0;
    while(1){
        int client_socket = accept(server.fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(client_socket <= 0){
            fprintf(stderr, "Error: %s\n", strerror(errno));
            continue;
        }

        while(1){
            int err;
            err = one_request(client_socket);
            if(err) break;

        };
        close(client_socket);
    };

    return 0;
}

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
