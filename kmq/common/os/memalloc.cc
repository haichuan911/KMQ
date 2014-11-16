/*
 *   @filename: 
 *   @author: Haichuan
 *   @email : neptune1990@126.com
 *   @date  : 
 *   @version: 
 */

#include <malloc.h>
#include <string.h>
#include "memalloc.h"

KMQ_DECLARATION_START


static mem_stat_t mem_stats = {};

void *
mem_alloc(uint32_t size)
{
    void  *p = malloc(size);
    if (p) {
	mem_stats.alloc++;
	mem_stats.alloc_size += size;
    }
    return p;
}

void *
mem_zalloc(uint32_t size)
{
    void  *p = mem_alloc(size);
    if (p) {
        memset(p, 0, size);
    }
    return p;
}

void *
mem_align(uint32_t alignment, uint32_t size)
{
    void  *p;
    p = memalign(alignment, size);
    if (p) {
	mem_stats.alloc++;
	mem_stats.alloc_size += size;
    }
    return p;
}


void mem_free(void *ptr) {
    if (ptr) {
	mem_stats.alloc--;
    }
    free(ptr);
}

void mem_free(void *ptr, uint32_t size) {
    if (ptr) {
	mem_stats.alloc--;
	mem_stats.alloc_size -= size;
    }
    free(ptr);
}

void *mem_realloc(void *ptr, uint32_t size) {
    void *newptr = NULL;
    if (!ptr)
	return mem_alloc(size);
    newptr = realloc(ptr, size);
    return newptr;
}


const mem_stat_t *mem_stat() {
    return &mem_stats;
}


}
