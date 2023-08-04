#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define ERR_LOG(s) fprintf(stderr, "Error: %s\n", s)

#define PORT 8080
#define HOST "127.0.0.1"
#define BACKLOG 64

int server_socket;

static void do_something(int client_socket);

int main(){
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    int ret, v = 1;
    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(int));

    if(ret < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;


    ret = inet_pton(AF_INET, HOST, &server_addr.sin_addr.s_addr);

    if(ret < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    ret = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    };

    ret = listen(server_socket, BACKLOG);
    if(ret < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = 0;
    while(1){
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if(client_socket <= 0){
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(1);
        }

        do_something(client_socket);
        close(client_socket);
    };

    return 0;
}

static void do_something(int client_socket){
    char buffer[64 + 1];
    memset(buffer, 0, sizeof(char) * 64);

    ssize_t n;
    n = read(client_socket, buffer, 64);
    if(n < 0){
        ERR_LOG(strerror(errno));
        return;
    }

    buffer[n] = '\0';
    printf("Client says: %s\n", buffer);

    const char *response = "Bitch";
    write(client_socket, response, strlen(response));
};


















