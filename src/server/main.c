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
#include <server/server.h>
#include <time.h>

#include "array.h"
#define PORT 8080
#define HOST "127.0.0.1"
#define BACKLOG 64

Server server;
struct Test{
    int x;
    char string[20 + 1];
};

const char *table = "abcdefghijk";
void make_rand_string(char *s){
    for(int i = 0; i < 20; i++){
        s[i] = table[rand() % strlen(table)];
    }
    s[20] = '\0';
}

void print_test(struct Test *t){
    printf("num : %d, string: %s\n", t->x, t->string);
}

int main(){
    rcpy_server_init(&server, HOST, PORT);
    rcpy_server_start_loop(&server);
    rcpy_server_quit(&server);
}
