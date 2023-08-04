#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"

#define ERR_LOG(s) fprintf(stderr, "Error: %s\n", s);
#define DIE(s) fprintf(stderr, "Error: %s\n", s); exit(1);

int client_socket;

int main(){
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0){
        printf("wtf\n");
        DIE(strerror(errno));
    }

    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_family = AF_INET;

    int ret;
    ret = inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr.s_addr);
    if(ret < 0){
        printf("wtf\n");
        DIE(strerror(errno));
    }

    ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0){
        printf("wtf\n");
        DIE(strerror(errno));
    }

    // we're connected!
    const char *msg = "Hello server!";
    write(client_socket, msg, strlen(msg));

    char response[128 + 1];
    memset(response, 0, 128 + 1);

    ssize_t n;
    n = read(client_socket, response, 128);
    if(n < 0){
        DIE(strerror(errno));
    }

    printf("Server reply: %s\n", response);
    close(client_socket);
}
