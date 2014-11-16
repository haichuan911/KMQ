#ifndef _H_MSGQUEUE_
#define _H_MSGQUEUE_


#include "rbtree.h"
#include "list.h"
#include "proto.h"
#include "mq_stat_module.h"


using namespace std;

KMQ_DECLARATION_START

enum {
    QUEUE_REQ = 0x01,
    QUEUE_RESP = 0x02,
};


class msg_queue {
 public:
    msg_queue();
    virtual ~msg_queue();

    int Init(int qcap);
    inline int Ref() {
	ref++;
	return ref;
    }
    inline int unRef() {
	ref--;
	return ref;
    }
    inline int Type() {
	return typ;
    }
    inline uint32_t Size() {
	return size;
    }
    inline uint32_t Cap() {
	return cap;
    }
    int Push(struct kmqmsg *elem);
    struct kmqmsg *Pop();
    
 private:

    int ref, typ;
    uint32_t size;
    uint32_t cap;
    struct list_head massages_queue;
};



class msg_queue_monitor {
 public:
    msg_queue_monitor();
    virtual ~msg_queue_monitor() {}
    
    virtual int mq_empty_callback_func() = 0;
    virtual int mq_nonempty_callback_func() = 0;

    int __attach_to_mqmonitor_head(struct list_head *head);
    int __detach_from_mqmonitor_head();
    
 private:
    struct list_link link;
};

#define list_monitor(link) ((msg_queue_monitor *)link->self) 
#define list_first_monitor(head)					\
    ({struct list_link *__pos =						\
	    list_first(head, struct list_link, node); (msg_queue_monitor *)__pos->self;})

#define list_mq(link) ((MQueue *)link->self) 
#define list_first_mq(head)						\
    ({struct list_link *__pos =						\
	    list_first(head, struct list_link, node);  list_mq(__pos);})

 
class MQueue : public msg_queue {
 public:
    MQueue(const string &_appid, const string &_id);
    ~MQueue();
    friend class MQueuePool;

    inline module_stat *Stat() {
	return &qstat;
    }
    inline string Id() {
	return qid;
    }
    inline const char *cid() {
	return qid.c_str();
    }
    inline const char *cappid() {
	return appid.c_str();
    }
    int MonitorsNum();
    int AddMonitor(msg_queue_monitor *monitor);
    int DelMonitor(msg_queue_monitor *monitor);

    int Setup(int cap, int tw_msec, int to_msec);
    int TimeWaitTime() {
	return time_wait_msec;
    }
    int PushMsg(struct kmqmsg *header);
    struct kmqmsg *PopMsg();

    int attach_to_mqpool_head(struct list_head *head);
    int detach_from_mqpool_head();
    
 private:
    string qid, appid;
    int msg_to_msec, time_wait_msec, monitors_cnt;
    module_stat qstat;
    __mq_stat_module_trigger __mq_sm;
    rbtree_node_t tw_timeout_node;
    struct list_head monitors;
    struct list_link mqp_node;

    bool msg_is_timeout(struct kmqmsg *msg);
};



}



#endif   // _H_MSGQUEUE_
