#ifndef _H_MEM_STATUS_
#define _H_MEM_STATUS_

#include "memalloc.h"


KMQ_DECLARATION_START

typedef struct kmq_mem_status {
    mem_stat_t app_context;
    mem_stat_t kmq_config, app_config;
    mem_stat_t rolemanager, dispatchers, receivers;
    mem_stat_t lb_iphash, lb_random, lb_rrbin, lb_fair;
    mem_stat_t msgpoller;
    mem_stat_t msgqueuepool, msgqueue;
    mem_stat_t rgm, rgm_accepter, rgm_connector, rgm_register;
    mem_stat_t tdqueue, timer;
} kmq_mem_status_t;



}

















#endif  // _H_MEM_STATUS_
