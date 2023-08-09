#include <stdio.h>
#include <server/server.h>

#define PORT 8080
#define HOST "127.0.0.1"
#define BACKLOG 64

Server server;
int main(){
    server_init(&server, HOST, PORT);
    printf("Server running...\n");
    server_start_loop(&server);
    server_quit(&server);
}
