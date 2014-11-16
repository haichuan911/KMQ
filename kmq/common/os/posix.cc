#include <unistd.h>
#include "os.h"


KMQ_DECLARATION_START

uint32_t SLB_PAGESIZE;
uint32_t SLB_PAGESIZE_SHIFT;
uint32_t SLB_CACHELINE_SIZE;
uint64_t kmq_start_timestamp;

int kmq_os_init() {
    SLB_PAGESIZE = getpagesize();
    for (uint32_t n = SLB_PAGESIZE; n >>= 1; SLB_PAGESIZE_SHIFT++)
	{ /* void */ }
    kmq_start_timestamp = rt_mstime();
    return 0;
}


}
