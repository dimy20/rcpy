#include <cstddef>
#include <cstdio>
#include <string.h>
#include <server/server.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <queue>
#include <uv.h>
#include "conn.h"
#include "socket.h"
#include "error.h"

#define ARR_LEN(arr) sizeof(arr) / sizeof(arr[0])
#define MAX_CONNECTIONS 1024
#define MAX_ADDR_STRING_SIZE 256

typedef struct Server Server;

struct Server{
    Conn* connections[MAX_CONNECTIONS];
    size_t num_connections;
    char addr_str[MAX_ADDR_STRING_SIZE];
    uint16_t port;
    bool alive;
    uv_tcp_t stream;
};

static Server *server;

static bool server_start_loop_internal(Server *server);
static bool server_accept_conn(Server *server);
static void server_close_conn(Server *server, Conn *conn);
static bool server_setup_epoll_instance(Server *server);
static bool server_update_events(Server *server);
static void server_process_events(Server *server, size_t num_events);
static bool server_init_uv(Server *server);

bool server_init(const char *addr_string, uint16_t port){
    server = (Server *)RCPY_MALLOC(sizeof(Server));

    server->num_connections = 0;
    memset(server->connections, 0, sizeof(server->connections));
    
    strncpy(server->addr_str, addr_string, MAX_ADDR_STRING_SIZE);
    server->port = port;
    server->alive = true;

    return server_init_uv(server);
};

static void on_conn_closed(uv_handle_t* handle){
    assert(handle != NULL && handle->data);
    Conn *conn = (Conn *)handle->data;
    free(handle);
    free(conn);
}

static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    assert(handle != NULL);
    assert(buf != NULL);
    buf->base = (char *)RCPY_MALLOC(suggested_size);
    buf->len = suggested_size;
}

static void on_conn_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
    assert(stream != NULL && buf != NULL);

    Conn *conn = (Conn*)stream->data;
    assert(conn != NULL);

    if(nread == 0){ // EAGAIN?
        return;
    }else if(nread < 0){
        if(nread == UV_EOF){
            fprintf(stderr, "%s\n", uv_strerror(nread));
        }else{
            UV_ERR_LOG("on_conn_read", nread);
        }
        server_close_conn(server, conn);
    }else{
        conn_read_incoming_data(conn, (size_t)nread, buf);
        if(conn->state == CONN_STATE_END){
            server_close_conn(server, conn);
        }
    }
    free(buf->base);
};

static Conn * server_find_empty_spot_or_push(Server *server, int * at){
    assert(server->num_connections < MAX_CONNECTIONS);

    int empty_spot = -1;
    for(size_t i = 0; i < server->num_connections; i++){
        if(server->connections[i] == NULL){
            empty_spot = i;
        }
    }
    Conn *conn;
    if(empty_spot != -1){
        *at = empty_spot;
        conn = server->connections[empty_spot];
    }else{
        *at = server->num_connections;
        conn = server->connections[server->num_connections++];
    }

    conn = (Conn*)RCPY_MALLOC(sizeof(Conn));
    return conn;
}

static bool server_add_conn(Server * server, Conn *conn){
    if(server->num_connections > MAX_CONNECTIONS){
        return false;
    }

    assert(server->connections[conn->fd] == NULL);
    server->connections[conn->fd] = conn;
    return true;
};

static void on_new_connection(uv_stream_t* server_stream, int status){
    Server *server = (Server*)server_stream->data;
    assert(server != NULL);
    if(status < 0){
        UV_ERR_LOG("on_new_connection", status);
        return;
    }

    if(server->num_connections >= MAX_CONNECTIONS){
        fprintf(stderr, "Error: Maximum connections exceeded, cant accept new conneciton\n");
        uv_loop_close(uv_default_loop());
        return;
    }

    uv_tcp_t *client;
    client = (uv_tcp_t*)RCPY_MALLOC(sizeof(uv_tcp_t));

    int ret;
    if((ret = uv_tcp_init(uv_default_loop(), client)) < 0){
        UV_ERR_LOG("uv_tcp_init", ret);
        free(client);
        return;
    }

    if((ret = uv_accept(server_stream, (uv_stream_t*)client)) < 0){
        UV_ERR_LOG("uv_accept", ret);
        free(client);
        return;
    };

    LOG("New connection");

    //TODO: pass this down to Conn
    struct sockaddr_storage peer_name;
    int len =  sizeof(peer_name);
    
    if((ret = uv_tcp_getpeername(client, (struct sockaddr*)&peer_name, &len)) < 0){
        UV_ERR_LOG("uv_tcp_getpeername", ret);
        free(client);
        return;
    }

    Conn *conn;
    conn = (Conn*)RCPY_MALLOC(sizeof(Conn));
    if(!conn_init(conn, client)){
        free(client);
        free(conn);
        return;
    }
    assert(server_add_conn(server, conn));

    if((ret = uv_read_start((uv_stream_t*)client, alloc_cb, on_conn_read)) < 0){
        UV_ERR_LOG("uv_read_start", ret);
        server->connections[conn->fd] = NULL;
        free(conn);
        free(client);
    }
}

static bool server_init_uv(Server *server){
    int ret;
    if((ret = uv_tcp_init(uv_default_loop(), &server->stream)) < 0){
        UV_ERR_LOG("uv_tcp_init", ret);
        return false;
    };

    struct sockaddr_in server_address;
    if((ret = uv_ip4_addr(server->addr_str, server->port, &server_address)) < 0){
        UV_ERR_LOG("uv_ip4_addr", ret);
        return false;
    }

    if((ret = uv_tcp_bind(&server->stream, (const struct sockaddr*)&server_address, 0)) < 0){
        UV_ERR_LOG("uv_tcp_bind", ret);
        return false;
    }
    server->stream.data = server;
    return true;
}

void server_quit(){
    for(size_t i = 0; i < server->num_connections; i++){
        if(server->connections[i] != NULL){
            server_close_conn(server, server->connections[i]);
        };
    }
    free(server);
}

bool server_start_loop(){
    assert(server != NULL);
    return server_start_loop_internal(server);
};

static bool server_start_loop_internal(Server *server){
    int ret;
    if((ret = uv_listen((uv_stream_t*)&server->stream, 10, on_new_connection)) < 0){
        UV_ERR_LOG("uv_listen", ret);
        return false;
    }
    if((ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT)) < 0){
        UV_ERR_LOG("uv_run", ret);
        return false;
    }
    return uv_loop_close(uv_default_loop());
}

static void server_close_conn(Server *server, Conn *conn){
    assert(server != NULL && conn != NULL);
    server->connections[conn->fd] = NULL;
    uv_close((uv_handle_t *)conn->stream, on_conn_closed);
};
