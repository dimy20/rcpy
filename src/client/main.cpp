#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

#include "io.h"
#include "socket.h"

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"

#define ERR_LOG(s) fprintf(stderr, "Error: %s\n", s);
#define DIE(s) fprintf(stderr, "Error: %s\n", s); exit(1);

#define MAX_MSG_SIZE 4096
#define HEADER_SIZE 4

int client_socket;

// len msg len mesg
int query(int client_socket, const char *query){
    size_t query_len = strlen(query);
    if(query_len > MAX_MSG_SIZE){
        ERR_LOG("Query exceeds maximum length");
        return -1;
    }

    // build query
    char send_buffer[HEADER_SIZE + MAX_MSG_SIZE];

    // assuming little endian
    memcpy(send_buffer, &query_len, 4);
    memcpy(send_buffer + HEADER_SIZE, query, query_len);

    int err;
    err = write_all(client_socket, send_buffer, HEADER_SIZE + query_len);
    if(err){
        return err;
    }

    // read and parse response
    char recv_buffer[HEADER_SIZE + MAX_MSG_SIZE + 1];
    err = read_all(client_socket, recv_buffer, HEADER_SIZE);
    if(err){
        if(err == 0){
            ERR_LOG("EOF");
        }else{
            ERR_LOG(strerror(errno));
        }
        return err;
    }

    size_t response_len;
    memcpy(&response_len, recv_buffer, HEADER_SIZE);
    if(response_len > MAX_MSG_SIZE){
        ERR_LOG("Server response exceeds maximum message length");
        return -1;
    }

    err = read_all(client_socket, recv_buffer + 4, response_len);

    if(err){
        return err;
    }

    recv_buffer[response_len + 4] = '\0';
    printf("Server response: %s\n", recv_buffer + 4);

    return 0;
    // build the geade
};

const char *table = "abcdef";
std::string rand_str(int n){
    std::string s;
    for(int i = 0; i < n; i++){
        s += table[rand() % strlen(table)];
    }
    return s;
};

int main(){
    srand(time(0));

    net::socket_init(client_socket, 0);
    struct sockaddr_in server_addr;
    net::socket_addr_init(&server_addr, SERVER_ADDR, SERVER_PORT);

    int ret;
    ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0){
        DIE(strerror(errno));
    }
    // multiple requests
    for(int i = 0; i < 5; i++){
        std::string s = rand_str(rand() % 50);
        int32_t err = query(client_socket, s.c_str());
        if(err) break;
    }
    close(client_socket);
    return 0;
}
