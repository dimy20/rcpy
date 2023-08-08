
#include <stdio.h>
#include "error.h"
#include <stdio.h>

//TODO: Write tests for this!
static const size_t DARRAY_HEADER_SIZE = 2 * sizeof(size_t);
#define DARRAY_DEFAULT_CAP 32
#define CAP_INDEX 0
#define LEN_INDEX 1
/*
 * Non type safe dynamic array implemetation.
 *
 * <  header >
 * [cap, size, elem1, elem2, elem3]
 *           ^ base addr
 *
 * */

/* This function has to be called before doing : arr[len] = elem, it takes care
 * of allocating memory if array is empty, resizing and updating the metadata.
 * it advances the length by one prior to insertion, so caller should do arr[len - 1] = elem,
 * after the call.
 * num_elements arguments it's currently unused, im leaving here in case i might one to
 * add support for adding several elements at once.
 * */
void * rcpy_arr_update(void *arr, size_t num_elements, size_t element_size){
    if(arr == NULL){
        /* If arr is NULL then is the first time is being called.
         * Allocate space for elements, the header, and return the base address.
         * */
        size_t cap_bytes = (DARRAY_DEFAULT_CAP * element_size);
        size_t size = DARRAY_HEADER_SIZE + cap_bytes;
        arr = RCPY_MALLOC(size);

        ((size_t *)arr)[CAP_INDEX] = DARRAY_DEFAULT_CAP;
        ((size_t *)arr)[LEN_INDEX] = 1;

        return ((char *)arr) + DARRAY_HEADER_SIZE;
    }else{
        // arr is pointing to the base_addr, offset back to the header.
        size_t *arr_start = ((size_t *)arr) - 2;

        // Grow the array if necessary
        if(arr_start[LEN_INDEX] >= arr_start[CAP_INDEX]){
            size_t new_cap = arr_start[CAP_INDEX] * 2;
            size_t new_cap_bytes = new_cap * element_size;

            arr_start = (size_t *)realloc(arr_start, new_cap_bytes);
            //TODO: Handle failure
            arr_start[CAP_INDEX] = new_cap;
        }

        arr_start[LEN_INDEX]++;
        // return base address
        return arr_start + 2;
    }
}

void rcpy_arr_free(void *arr){
    if(arr == NULL) return;
    free(((char*)arr) - DARRAY_HEADER_SIZE);
}

size_t rcpy_arr_len(void * base_addr){
    if(base_addr == NULL) return 0;
    char * addr = ((char *)base_addr) - DARRAY_HEADER_SIZE;
    return ((size_t *)(addr))[LEN_INDEX];
};

