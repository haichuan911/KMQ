#ifndef _H_TIMER_
#define _H_TIMER_


#include "pmutex.h"
#include "epoller.h"
#include "rbtree.h"

KMQ_DECLARATION_START


typedef int (*timer_callback_func) (void *data);


class timer_callback;
class Timer {
 public:
    Timer();
    ~Timer();

    inline void Setup(int tcap) {
	cap = tcap;
    }
    int Wait(int to_msec);
    int AddTimerEvent(timer_callback_func func, void *data, int to_msec);
    
 private:
    Epoller *poller;
    int cap, size;
    vector<timer_callback *> idle_timer_cbs;
    vector<timer_callback *> busy_timer_cbs;
};


typedef void (*tdqueue_clean_func) (void *data);

// timer data queue
class TDQueue {
 public:
    TDQueue();
    ~TDQueue();

    int Lock();
    int unLock();
    void SetCleanFunc(tdqueue_clean_func clfunc) {
	cfunc = clfunc;
    }
    int PushTD(void *data, int to_msec);
    void *PopTD();
    
 private:
    pmutex_t lock;
    int size;
    tdqueue_clean_func cfunc;
    rbtree_t timer_rbtree;
    rbtree_node_t timer_sentinel;
};







}




#endif   // _H_TIMER_
