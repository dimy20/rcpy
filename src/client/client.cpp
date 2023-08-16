#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <iostream>

#include <vector>
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

const char *table = "abcdef";
void rand_str(char * buffer, int n){
    for(int i = 0; i < n; i++){
        buffer[i] = table[rand() % strlen(table)];
    }
};

//void prepare_buffer()

size_t count_msg_size(const std::vector<std::string>& cmd){
    size_t n = 4;
    for(auto& s : cmd){
        n += 4 + s.size();
    }
    return n;
}

int main(int argc, char **argv){
    srand(time(0));

    socket_init(&client_socket, 0);
    struct sockaddr_in server_addr;
    socket_addr_init(&server_addr, SERVER_ADDR, SERVER_PORT);

    int ret;
    ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0){
        DIE(strerror(errno));
    }

    if(argc < 2){
        exit(1);
    }

    std::vector<std::string> cmd;
    for(int i = 1 ; i < argc; i++){
        cmd.push_back(argv[i]);
    }

    uint32_t _msg_size = count_msg_size(cmd);
    uint32_t msg_size = htonl(count_msg_size(cmd));
    uint32_t num_strings = htonl(cmd.size());

    char buffer[MAX_MSG_SIZE + 4];
    memcpy(buffer, &msg_size, 4);
    memcpy(buffer + 4, &num_strings, 4);

    size_t pos = 8;
    for(auto& s : cmd){
        uint32_t n = htonl(s.size());
        memcpy(buffer + pos, &n, 4);
        memcpy(buffer + pos + 4, s.c_str(), s.size());
        pos += s.size() + 4;
    }

    printf("Sending : %d\n", _msg_size + 4);
    write_all(client_socket, buffer, _msg_size + 4);

    //char read_buffer[MAX_MSG_SIZE + 4];
    //read_all(client_socket, read_buffer, );
}
