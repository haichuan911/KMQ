#include "os.h"
#include "log.h"
#include "timer.h"
#include "mem_status.h"


KMQ_DECLARATION_START

extern kmq_mem_status_t kmq_mem_stats;

static mem_stat_t *tdqueue_mem_stats = &kmq_mem_stats.tdqueue;
static mem_stat_t *timer_mem_stats = &kmq_mem_stats.timer;

class timer_callback {
public:
    timer_callback();
    ~timer_callback();
    friend class Timer;

    int Setup(timer_callback_func func, void *data);
    int EnableEvent(Epoller *poller, int to_msec);
    int DisableEvent(Epoller *poller);

    
private:
    timer_callback_func cb_func;
    void *cb_data;
    EpollEvent ee;
};



timer_callback::timer_callback() :
    cb_data(NULL)
{
    timer_mem_stats->alloc++;
    timer_mem_stats->alloc_size += sizeof(timer_callback);
}


timer_callback::~timer_callback() {
    timer_mem_stats->alloc--;
    timer_mem_stats->alloc_size -= sizeof(timer_callback);
}


int timer_callback::Setup(timer_callback_func func, void *data) {
    cb_func = func;
    cb_data = data;
    return 0;
}

int timer_callback::EnableEvent(Epoller *poller, int to_msec) {
    int ret = 0;
    if (!poller) {
	KMQLOG_WARN("unexpect poller [%p]", poller);
	return -1;
    }
    ee.ptr = this;
    ee.to_nsec = to_msec * 1000000;
    ret = poller->CtlAdd(&ee);
    return ret;
}


int timer_callback::DisableEvent(Epoller *poller) {
    int ret;
    if (!poller) {
	KMQLOG_WARN("unexpect poller [%p]", poller);
	return -1;
    }
    ret = poller->CtlDel(&ee);
    return ret;
}








Timer::Timer() :
    poller(NULL), cap(0), size(0)
{
    poller = EpollCreate(1024, 500);
    if (!poller) {
	KMQLOG_ERROR("unexpect out of memory");
    }
    timer_mem_stats->alloc++;
    timer_mem_stats->alloc_size += sizeof(Timer);
}


Timer::~Timer() {
    vector<timer_callback *>::iterator it;

    for (it = idle_timer_cbs.begin(); it != idle_timer_cbs.end(); ++it) {
	delete *it;
    }
    for (it = busy_timer_cbs.begin(); it != busy_timer_cbs.end(); ++it) {
	delete *it;
    }
    if (poller)
	delete poller;
    timer_mem_stats->alloc--;
    timer_mem_stats->alloc_size -= sizeof(Timer);
}


int Timer::AddTimerEvent(timer_callback_func func, void *data, int to_msec) {
    int ret = 0;
    timer_callback *cb = NULL;
    if (size >= cap) {
	KMQLOG_ERROR("timer queue is full");
	return -1;
    }
    
    if (!idle_timer_cbs.empty()) {
	cb = idle_timer_cbs.back();
	idle_timer_cbs.pop_back();
    } else {
	cb = new (std::nothrow) timer_callback();
    }
    if (!cb) {
	KMQLOG_ERROR("unexpect out of memory");
	return -1;
    }
    size++;
    cb->Setup(func, data);
    ret = cb->EnableEvent(poller, to_msec);
    return ret;
}


int Timer::Wait(int to_msec) {
    int ret = 0;
    EpollEvent *ev = NULL;
    timer_callback *cb = NULL;
    struct list_head io_head, to_head;

    INIT_LIST_HEAD(&io_head);
    INIT_LIST_HEAD(&to_head);

    if ((ret = poller->Wait(&io_head, &to_head, to_msec)) < 0) {
	KMQLOG_ERROR("timer wait events");
	return -1;
    }

    if (!list_empty(&io_head)) {
	KMQLOG_ERROR("unexpect io events ? ...");
    }

    list_for_each_poll_link_autodetach(ev, &to_head) {
	size--;
	cb = (timer_callback *)ev->ptr;
	cb->cb_func(cb->cb_data);
	idle_timer_cbs.push_back(cb);
    }}}
detach_for_each_poll_link(&io_head);
detach_for_each_poll_link(&to_head);

return 0;
}




TDQueue::TDQueue() :
    size(0), cfunc(NULL)
{
    pmutex_init(&lock);
    rbtree_init(&timer_rbtree, &timer_sentinel, rbtree_insert_timer_value);
    tdqueue_mem_stats->alloc++;
    tdqueue_mem_stats->alloc_size += sizeof(TDQueue);
}

TDQueue::~TDQueue() {
    rbtree_node_t *node = NULL;
    
    while (timer_rbtree.root != &timer_sentinel) {
	node = rbtree_min(timer_rbtree.root, &timer_sentinel);
	rbtree_delete(&timer_rbtree, node);
	if (cfunc)
	    cfunc(node->data);
	mem_free(node, sizeof(*node));
    }
    pmutex_destroy(&lock);
    tdqueue_mem_stats->alloc--;
    tdqueue_mem_stats->alloc_size -= sizeof(TDQueue);
}



int TDQueue::Lock() {
    pmutex_lock(&lock);
    return 0;
}


int TDQueue::unLock() {
    pmutex_unlock(&lock);
    return 0;
}


int TDQueue::PushTD(void *data, int to_msec) {
    rbtree_node_t *node = NULL;

    node = (rbtree_node_t *)mem_zalloc(sizeof(*node));
    if (!node) {
	KMQLOG_ERROR("unexpect out of memory");
	return -1;
    }
    node->key = rt_mstime() + to_msec;
    node->data = data;
    rbtree_insert(&timer_rbtree, node);
    size++;
    return 0;
}


void *TDQueue::PopTD() {
    rbtree_node_t *node = NULL;
    void *data = NULL;
    int64_t cur_ms = rt_mstime();
    
    if (timer_rbtree.root == &timer_sentinel)
	return NULL;
    node = rbtree_min(timer_rbtree.root, &timer_sentinel);
    if (node->key > cur_ms)
	return NULL;
    data = node->data;
    rbtree_delete(&timer_rbtree, node);
    mem_free(node, sizeof(*node));
    size--;
    
    return data;
}



}
