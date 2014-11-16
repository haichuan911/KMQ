#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include "memalloc.h"
#include "osthread.h"
#include "mempool.h"

using namespace kmq;


static int test_alloc() {
    void *ptr;
    const mem_stat_t *status;
    int i, cnt, size;

    for (i = 0, cnt = 100000; i < cnt; i++) {
	ptr = NULL;
	size = rand() % (SLB_PAGESIZE * 2);
	switch (size % 4) {
	case 3:
	    if (!(ptr = mem_realloc(ptr, size)))
		continue;
	    break;
	case 2:
	    if (!(ptr = mem_align(sizeof(long), size)))
		continue;
	    if (0 != ((long)ptr % sizeof(long)))
		return -1;
	    break;
	case 1:
	    if (!(ptr = mem_alloc(size)))
		continue;
	    break;
	case 0:
	    if (!(ptr = mem_zalloc(size)))
		continue;
	    break;
	}
	mem_free(ptr);
    }
    status = mem_stat();
    return 0;
}


TEST(memalloc, alloc) {
    test_alloc();
}

