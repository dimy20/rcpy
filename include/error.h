#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define LOG(s) fprintf(stderr, "%s\n", s)
#define ERR_LOG(s) fprintf(stderr, "Error: %s\n", s)
#define DIE(s) fprintf(stderr, "Error: %s\n", s); exit(1);
#define DIE_ERRNO() fprintf(stderr, "Error: %s\n", strerror(errno)); exit(1);

#define RCPY_MALLOC(size)                     \
    ({                                        \
        void *mem = malloc(size);             \
        if(mem == NULL){                      \
            DIE("Memory allocation failed");  \
        }                                     \
        mem;                                  \
    })

#define UV_ERR_LOG(s, r) do{ \
    fprintf(stderr, "Error %s: %s\n", s, uv_strerror(r)); \
}while(0);

