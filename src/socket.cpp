#include "socket.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <cassert>

#define BACKLOG 64

bool net::socket_init(int& fd, uint32_t flags){
    fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return false;
    }

    if(flags & SockFlag::NON_BLOCK){
        int ret, v = 1;

        ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(int));
        if(ret < 0){
            fprintf(stderr, "Error: %s\n", strerror(errno));
            return false;
        }

    }
    return true;
}

bool net::socket_make_nonblock(int socket){
    errno = 0;
    int flags = fcntl(socket, F_GETFL, 0);
    if(errno){
        return false;
    }

    flags |= O_NONBLOCK;
    errno = 0;
    fcntl(socket, F_SETFL, flags);

    if(errno){
        return false;
    }

    return true;
};

bool net::socket_listen(int fd, const char *host, uint16_t port){
    struct sockaddr_in server_addr;
    socket_addr_init(&server_addr, host, port);

    int ret;
    ret = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return false;
    };

    ret = listen(fd, BACKLOG);

    if(ret < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return false;
    }
    return true;
};

bool net::socket_addr_init(struct sockaddr_in *addr, const char *host, uint16_t port){
    assert(addr != NULL);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    errno = 0;
    if(inet_pton(AF_INET, host, &addr->sin_addr.s_addr) < 0) return false;
    return true;
}
