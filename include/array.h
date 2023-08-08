#pragma once

#include <stdio.h>
void *rcpy_arr_update(void *base_adder, size_t num_elements, size_t element_size);
size_t rcpy_arr_len(void * base_addr);
void rcpy_arr_free(void *base_addr);

#define rcpy_arr_push(base_addr, elem) do{ \
    (base_addr) = rcpy_arr_update(base_addr, 1, sizeof(base_addr[0]));\
    (base_addr)[rcpy_arr_len(base_addr) - 1] = (elem); \
}while(0);

