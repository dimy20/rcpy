#include <string.h>
#include <server/server.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "conn.h"
#include "socket.h"
#include "error.h"

#define ARR_LEN(arr) sizeof(arr) / sizeof(arr[0])

static bool server_start_loop_internal(Server *server);
static bool server_accept_conn(Server *server);
static void server_close_conn(Server *server, Conn *conn);
static bool server_setup_epoll_instance(Server *server);

bool server_init(Server *server, const char *addr_string, uint16_t port){
    if(!socket_init(&server->fd, SOCKET_NON_BLOCK)) return false;

    socket_make_nonblock(server->fd);
    server->num_connections = 0;
    memset(server->connections, 0, sizeof(server->connections));
    
    strncpy(server->addr_str, addr_string, MAX_ADDR_STRING_SIZE);
    server->port = port;
    server->alive = true;

    return socket_listen(server->fd, addr_string, port) &&
           server_setup_epoll_instance(server);
};

void server_quit(Server *server){}

static bool server_accept_conn(Server *server){
    int client_fd;
    client_fd = accept(server->fd, NULL, NULL);

    if(client_fd < 0){
        perror("accept");
        return false;
    }

    Conn *new_conn = (Conn*)RCPY_MALLOC(sizeof(Conn));
    if(!conn_init(new_conn, client_fd)){
        close(client_fd);
        free(new_conn);
        return false;
    }

    if(client_fd >= ARR_LEN(server->connections)){
        LOG("maximun number of connections exceeded");
        return false;
    }

    // register fd
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = client_fd;
    ev.events |= EPOLLIN;

    if(epoll_ctl(server->efd, EPOLL_CTL_ADD, client_fd, &ev) < 0){
        perror("epoll_ctl");
        return false;
    }

    server->connections[client_fd] = new_conn;
    return true;
};

bool server_start_loop(Server *server){
    assert(server != NULL);
    return server_start_loop_internal(server);
};

static bool server_start_loop_internal(Server *server){
    // main loop
    while(server->alive){
        int num_events;
        num_events = epoll_wait(server->efd, server->events, MAX_CONNECTIONS, -1);
        if(num_events < 0){
            perror("epoll_wait");
            return false;
        }

        for(size_t i = 0; i < num_events; i++){
            if(server->events[i].data.fd == server->fd){
                server_accept_conn(server);
            }else{
                int conn_fd = server->events[i].data.fd;
                Conn* conn = server->connections[conn_fd];
                assert(conn != NULL);

                if(server->events[i].events & EPOLLIN){
                    conn_io_read_many(conn);
                }

                if(conn->state == CONN_STATE_END){
                    server_close_conn(server, conn);
                };
            }
        }
    };
    return true;
}

static void server_close_conn(Server *server, Conn *conn){
    assert(server != NULL && conn != NULL);
    assert(conn->fd > 0 && conn->fd < MAX_CONNECTIONS);
    assert(server->connections[conn->fd] != NULL);

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = conn->fd;

    if(epoll_ctl(server->efd, EPOLL_CTL_DEL, conn->fd, &ev) < 0){
        perror("epoll_ctl");
    }

    close(conn->fd);
    server->connections[conn->fd] = NULL;
    free(conn);
};

static bool server_setup_epoll_instance(Server *server){
    server->efd = epoll_create1(0);

    if(server->efd < 0){
        ERR_LOG(strerror(errno));
        return false;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = server->fd;
    ev.events = EPOLLIN;

    if(epoll_ctl(server->efd, EPOLL_CTL_ADD, server->fd, &ev) < 0){
        ERR_LOG(strerror(errno));
        return false;
    }

    memset(server->events, 0, sizeof(server->events));
    return true;
}
