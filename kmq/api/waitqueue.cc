#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "waitqueue.h"


KMQ_DECLARATION_START

WaitQueue::WaitQueue() :
    qcap(1024), qsize(0), qflags(0), cleanup_func(NULL)
{
    INIT_LIST_HEAD(&mq);
    pmutex_init(&lock);
    pcond_init(&cond);
}


WaitQueue::~WaitQueue() {
    struct appmsg *raw = NULL, *tmp = NULL;

    list_for_each_appmsg_safe(raw, tmp, &mq) {
	if (cleanup_func)
	    cleanup_func(raw);
    }
    pmutex_destroy(&lock);
    pcond_init(&cond);
}


int WaitQueue::Setup(int cap, waitqueue_cleanup_func cfunc) {
    qcap = cap;
    cleanup_func = cfunc;
    return 0;
}

int WaitQueue::Push(struct appmsg *raw) {
    if (qcap > 0 && qsize >= qcap)
	return -1;
    list_add_tail(&raw->node, &mq);
    qsize++;
    return 0;
}


struct appmsg *WaitQueue::Pop() {
    struct appmsg *raw = NULL;

    if (!list_empty(&mq)) {
	raw = list_first_appmsg(&mq);
	list_del(&raw->node);
	qsize--;
    }
    return raw;
}

}    
