#ifndef __OS_H_INCLUDED__
#define __OS_H_INCLUDED__


#include <inttypes.h>
#include <sys/syscall.h>
#include "decr.h"

KMQ_DECLARATION_START


#define gettid() syscall(__NR_gettid)
    
extern uint32_t  SLB_PAGESIZE;
extern uint32_t  SLB_PAGESIZE_SHIFT;
extern uint32_t  SLB_CACHELINE_SIZE;


int kmq_os_init();

extern uint64_t kmq_start_timestamp;
int64_t clock_mstime(); // clock_gettime version
int64_t rt_mstime();    // gettimeofday version
int64_t clock_ustime(); // clock_gettime version
int64_t rt_ustime();    // gettimeofday version
int64_t clock_nstime(); // clock_gettime version
int64_t rt_nstime();    // gettimeofday version
 int rt_usleep(int64_t usec);

}




#endif
