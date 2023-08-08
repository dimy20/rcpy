#include <string.h>
#include <server/server.h>
#include <assert.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "conn.h"
#include "socket.h"
#include "error.h"

#define ARR_LEN(arr) sizeof(arr) / sizeof(arr[0])

static bool running = true;
static bool rcpy_server_start_loop_internal(Server *server);
static void rcpy_server_prepare_poll_args(Server *server);
static bool rcpy_server_accept_connection(Server *server);

bool rcpy_server_init(Server *server, const char *addr_string, uint16_t port){
    if(!socket_init(&server->fd, SOCKET_NON_BLOCK)) return false;

    socket_make_nonblock(server->fd);
    server->num_connections = 0;

    memset(server->connections, 0, sizeof(server->connections));
    memset(server->poll_args, 0, sizeof(server->poll_args));

    strncpy(server->addr_str, addr_string, MAX_ADDR_STRING_SIZE);
    server->port = port;

    return socket_listen(server->fd, addr_string, port);
};

void rcpy_server_quit(Server *server){

}

static void rcpy_server_prepare_poll_args(Server *server){
    assert(server != NULL);
    memset(server->poll_args, 0, sizeof(server->poll_args));
    // init poll arg for the server socket at index 0

    server->poll_args[0].fd = server->fd;
    server->poll_args[0].revents = POLLIN;

    // init poll arg for each active connection
    //size_t j = 1;
    /*
    for(size_t i = 0; i < ARR_LEN(server->connections); i++){
        Conn *conn = server->connections[i];
        if(!conn){
            continue;
        }
        server->poll_args[j].fd = conn->fd;
        server->poll_args[j].revents = conn->state == CONN_STATE_REQUEST ? POLLIN : POLLOUT;
        //server->poll_args[j].revents |= POLLERR;
        j++;
    }
    */
};

static bool rcpy_server_accept_connection(Server *server){

    int client_fd;
    client_fd = accept(server->fd, NULL, NULL);

    if(client_fd < 0){
        ERR_LOG("accept");
        return false;
    }

    Conn *new_conn = (Conn*)RCPY_MALLOC(sizeof(Conn));
    if(!rcpy_conn_init(new_conn, client_fd)){
        close(client_fd);
        free(new_conn);
        return false;
    }

    if(client_fd >= ARR_LEN(server->connections)){
        //TODO: Currently we support a fixed small number of connections
        // Change this to a dynamic array
        LOG("maximun number of connections exceeded");
        return false;
    }
    server->connections[client_fd] = new_conn;
    return true;
};

bool rcpy_server_start_loop(Server *server){
    assert(server != NULL);
    return rcpy_server_start_loop_internal(server);
};

static bool rcpy_server_start_loop_internal(Server *server){
    //struct sockaddr_storage client_addr;
    //socklen_t addr_size = sizeof(client_addr);

    // main loop
    while(running){
        rcpy_server_prepare_poll_args(server);
        int ret;
        printf("blocked\n");
        ret = poll(server->poll_args, server->num_connections + 1, -1);

        if(ret < 0){
            LOG("poll");
            DIE_ERRNO();
        }

        if(server->poll_args[0].revents){
            rcpy_server_accept_connection(server);
        };
    };
    return true;
};
