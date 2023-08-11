#include <stdio.h>
#include <server/server.h>

#define PORT 8080
#define HOST "127.0.0.1"
#define BACKLOG 64

int main(){
    server_init(HOST, PORT);
    printf("Server running...\n");
    server_start_loop();
    server_quit();
}
