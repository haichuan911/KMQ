#ifndef _KMQ_WAITQUEUE_H_
#define _KMQ_WAITQUEUE_H_

#include "tmutex.h"
#include "pcond.h"
#include "api.h"

using namespace std;

KMQ_DECLARATION_START

typedef int (*waitqueue_cleanup_func)(struct appmsg *);
    
class WaitQueue {
 public:
    WaitQueue();
    ~WaitQueue();
    int Setup(int cap, waitqueue_cleanup_func cfunc);
    void Lock() {
	pmutex_lock(&lock);
    }
    void UnLock() {
	pmutex_unlock(&lock);
    }
    void Wait() {
	pcond_wait(&cond, &lock);
    }
    void BroadCast() {
	pcond_broadcast(&cond);
    }
    bool Full() {
	return qsize == qcap;
    }
    bool Empty() {
	return qsize == 0;
    }
    int Push(struct appmsg *raw);
    struct appmsg *Pop();
    int BatchPop(struct list_head *head, int size);
    int BatchPush(struct list_head *head);

 private:
    int qcap, qsize, qflags;
    pmutex_t lock;
    pcond_t cond;
    struct list_head mq;
    waitqueue_cleanup_func cleanup_func;
};









}


#endif   // _KMQ_WAITQUEUE_H_
