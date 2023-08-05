#pragma once

#include <stdlib.h>
#include <sys/types.h>

ssize_t read_all(int socket, char * buffer, size_t n);
ssize_t write_all(int socket, const char * buffer, size_t n);
