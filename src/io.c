#include "io.h"
#include <stddef.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

ssize_t read_all(int socket, char * buffer, size_t n){
    size_t offset = 0;
    size_t read_so_far = 0;
    while(read_so_far < n){
        ssize_t bytes_read = read(socket, buffer + offset, n - read_so_far);

        if(bytes_read <= 0){
            return -1;
        }

        read_so_far += (size_t)bytes_read;
        offset += (size_t)bytes_read;
    };
    return 0;
}

ssize_t write_all(int socket, const char * buffer, size_t n){
    size_t offset = 0;
    while(n > 0){
        ssize_t bytes_sent;
        bytes_sent = write(socket, buffer + offset, n);

        if(bytes_sent <= 0){
            return -1;
        }

        assert(n >= (size_t)bytes_sent);
        n -= (size_t)bytes_sent;
        offset += (size_t)bytes_sent;
    }
    return 0;
}
