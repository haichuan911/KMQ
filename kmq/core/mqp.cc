#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "os.h"
#include "log.h"
#include "mem_status.h"
#include "mqp.h"


KMQ_DECLARATION_START
    
extern kmq_mem_status_t kmq_mem_stats;
static mem_stat_t *msgqueuepool_mem_stats = &kmq_mem_stats.msgqueuepool;






MQueuePool::MQueuePool()
{
    memset(&conf, 0, sizeof(conf));
    rbtree_init(&tw_timeout_tree, &tw_sentinel, rbtree_insert_timer_value);
    INIT_LIST_HEAD(&time_wait_queues);
    msgqueuepool_mem_stats->alloc++;
    msgqueuepool_mem_stats->alloc_size += sizeof(MQueuePool);
}

MQueuePool::~MQueuePool() {
    MQueue *mq = NULL;
    map<string, MQueue *>::iterator it;
    struct list_link *pos = NULL, *next = NULL;

    for (it = queues.begin(); it != queues.end(); ++it)
	delete it->second;
    list_for_each_list_link_safe(pos, next, &time_wait_queues) {
	mq = list_mq(pos);
	delete mq;
    }
    msgqueuepool_mem_stats->alloc--;
    msgqueuepool_mem_stats->alloc_size -= sizeof(MQueuePool);
}


int MQueuePool::Setup(const string &_appid, struct mqp_conf *_conf) {
    appid = _appid;
    conf = *_conf;
    return 0;
}


inline void MQueuePool::clean_tw_queues() {
    rbtree_node_t *node = NULL;
    MQueue *mq = NULL;
    int64_t cur_ms = rt_mstime();

    while (tw_timeout_tree.root != &tw_sentinel) {
	node = rbtree_min(tw_timeout_tree.root, &tw_sentinel);
	if (node->key > cur_ms)
	    break;
	mq = (MQueue *)node->data;
	rbtree_delete(&tw_timeout_tree, node);
	mq->detach_from_mqpool_head();
	KMQLOG_NOTICE("%s delete time_wait queue: %s", cappid(), mq->cid());
	delete mq;
    }
}


inline void MQueuePool::insert_tw_queue(MQueue *mq) {
    mq->attach_to_mqpool_head(&time_wait_queues);
    mq->tw_timeout_node.key = mq->TimeWaitTime() + rt_mstime();
    mq->tw_timeout_node.data = mq;
    rbtree_insert(&tw_timeout_tree, &mq->tw_timeout_node);
}


// find and erase from list
inline MQueue *MQueuePool::find_tw_queue(const string &qid) {
    MQueue *mq = NULL;
    struct list_link *pos = NULL, *next = NULL;

    list_for_each_list_link_safe(pos, next, &time_wait_queues) {
	mq = list_mq(pos);
	if (mq->Id() == qid) {
	    mq->detach_from_mqpool_head();
	    rbtree_delete(&tw_timeout_tree, &mq->tw_timeout_node);
	    break;
	}
	mq = NULL;
    }
    return mq;
}

MQueue *MQueuePool::Set(const string &qid) {
    MQueue *mq;
    map<string, MQueue *>::iterator it;

    clean_tw_queues();
    if ((it = queues.find(qid)) != queues.end()) {
	mq = it->second;
	KMQLOG_NOTICE("%s found an exist queue: %s", cappid(), mq->cid());
	mq->Ref();
	return mq;
    }
    // time wait ???
    if ((mq = find_tw_queue(qid)) != NULL) {
	KMQLOG_NOTICE("%s found an time_wait queue: %s", cappid(), mq->cid());
	queues.insert(make_pair(qid, mq));
	mq->Ref();
	return mq;
    }
    if (!(mq = new (std::nothrow) MQueue(appid, qid))) {
	KMQLOG_ERROR("%s out of memory", cappid());
	return NULL;
    }
    mq->Setup(conf.mq_cap, conf.mq_timewait_msec, conf.msg_timeout_msec);
    queues.insert(make_pair(qid, mq));
    mq->Ref();
    return mq;
}

int MQueuePool::unSet(const string &qid) {
    int ret;
    MQueue *mq = NULL;
    map<string, MQueue *>::iterator it;

    if ((it = queues.find(qid)) == queues.end()) {
	KMQLOG_WARN("%s queue %s doesn't exists", cappid(), qid.c_str());
	return -1;
    }
    mq = it->second;
    if ((ret = mq->unRef()) == 0) {
	queues.erase(it);
	if (mq->TimeWaitTime() == 0) {
	    KMQLOG_NOTICE("%s queue %s deleted", cappid(), mq->cid());
	    delete mq;
	} else {
	    KMQLOG_NOTICE("%s queue %s enter time_wait", cappid(), mq->cid());
	    insert_tw_queue(mq);
	}
    }
    return ret;
}


MQueue *MQueuePool::Find(const string &qid) {
    map<string, MQueue *>::iterator it;
    MQueue *mq = NULL;

    if ((it = queues.find(qid)) != queues.end())
	mq = it->second;

    return mq;
}

int MQueuePool::Walk(mq_walkfn walkfn, void *data) {
    map<string, MQueue *>::iterator it;

    for (it = queues.begin(); it != queues.end(); ++it)
	walkfn(it->second, data);
    return 0;
}

    

// Output all massage queue status
int MQueuePool::OutputQueueStatus(FILE *fp) {

    MQueue *mq = NULL;
    map<string, MQueue *>::iterator it;
    struct list_link *pos = NULL, *next = NULL;
    string uid;
    
    fprintf(fp, "------------------------------------------------------------\n");
    fprintf(fp,
	    "%15s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n\n",
	    "qid", "cap", "size", "passed", "dropped", "max", "min", "all",
	    "avg", "max_to", "min_to", "all_to", "avg_to", "monitor");

#define __output_queue_status(__uid, __mq)				\
    do {								\
	module_stat *__qstat = __mq->Stat();				\
	fprintf(fp, "%15s %10"PRId64" %10"PRId64" %10"PRId64		\
		" %10"PRId64" %10"PRId64" %10"PRId64" %10"PRId64	\
		" %10"PRId64" %10"PRId64" %10"PRId64" %10"PRId64	\
		" %10"PRId64" %10d\n",					\
		__uid.c_str(),						\
		__qstat->getkey_s(CAP),					\
		__qstat->getkey_s(SIZE),				\
		__qstat->getkey_s(PASSED),				\
		__qstat->getkey_s(DROPPED),				\
		__qstat->getkey_s(MAX_MSEC),				\
		__qstat->getkey_s(MIN_MSEC),				\
		__qstat->getkey_s(ALL_MSEC),				\
		__qstat->getkey_s(AVG_MSEC),				\
		__qstat->getkey_s(MAX_TO_MSEC),				\
		__qstat->getkey_s(MIN_TO_MSEC),				\
		__qstat->getkey_s(ALL_TO_MSEC),				\
		__qstat->getkey_s(AVG_TO_MSEC), __mq->MonitorsNum());	\
    } while (0)

    
    for (it = queues.begin(); it != queues.end(); ++it) {
	mq = it->second;
	uid.clear();
	uid.assign(mq->cid(), 8);
	__output_queue_status(uid, mq);
    }
    list_for_each_list_link_safe(pos, next, &time_wait_queues) {
	mq = list_mq(pos);
	uid.clear();
	uid.assign(mq->cid(), 8);
	uid += "(tw)";
	__output_queue_status(uid, mq);
    }
    fprintf(fp, "------------------------------------------------------------\n");

    return 0;
}


}
