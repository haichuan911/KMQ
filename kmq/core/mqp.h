#ifndef _H_MSGQUEUEEX_
#define _H_MSGQUEUEEX_

#include <map>
#include "mq.h"

KMQ_DECLARATION_START


struct mqp_conf {
    int mq_cap;
    int mq_timewait_msec;
    int msg_timeout_msec;
};

typedef int (*mq_walkfn) (MQueue *mq, void *data);
 
class MQueuePool {
 public:
    MQueuePool();
    ~MQueuePool();

    inline const char *cappid() {
	return appid.c_str();
    }
    int Setup(const string &_appid, struct mqp_conf *_conf);
    MQueue *Set(const string &qid);
    int unSet(const string &qid);
    MQueue *Find(const string &qid);
    int Walk(mq_walkfn walkfn, void *data);
    int OutputQueueStatus(FILE *fp);
    
 private:
    string appid;
    struct mqp_conf conf;
    map<string, MQueue *> queues;
    struct list_head time_wait_queues;
    rbtree_t tw_timeout_tree;
    rbtree_node_t tw_sentinel;

    inline void clean_tw_queues();
    inline void insert_tw_queue(MQueue *mq);
    inline MQueue *find_tw_queue(const string &qid);
};


}

#endif   // _H_MSGQUEUEEX_
