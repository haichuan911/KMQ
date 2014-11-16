#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "os.h"
#include "log.h"
#include "mq.h"
#include "mem_status.h"


KMQ_DECLARATION_START
    
extern kmq_mem_status_t kmq_mem_stats;
static mem_stat_t *msgqueue_mem_stats = &kmq_mem_stats.msgqueue;

    
msg_queue::msg_queue() :
    ref(0), typ(0), size(0), cap(0)
{
    INIT_LIST_HEAD(&massages_queue);
}

msg_queue::~msg_queue() {

}


int msg_queue::Init(int qcap) {
    ref = 0;
    cap = qcap;
    return 0;
}

int msg_queue::Push(struct kmqmsg *elem) {
    if (Size() >= cap) {
	return -1;
    }
    size++;
    list_add_tail(&elem->mq_node, &massages_queue);
    return 0;
}


struct kmqmsg *msg_queue::Pop() {
    struct kmqmsg *elem = NULL;

    if (!list_empty(&massages_queue)) {
	elem = list_first(&massages_queue, struct kmqmsg, mq_node);
	list_del(&elem->mq_node);
	size--;
    }
    return elem;
}


msg_queue_monitor::msg_queue_monitor() {
    INIT_LIST_LINK(&link);
}

int msg_queue_monitor::__attach_to_mqmonitor_head(struct list_head *head) {
    if (!link.linked) {
	link.linked = 1;
	list_add(&link.node, head);
	return 0;
    }
    return -1;
}

int msg_queue_monitor::__detach_from_mqmonitor_head() {
    if (link.linked) {
	list_del(&link.node);
	link.linked = 0;
	return 0;
    }
    return -1;
}

MQueue::MQueue(const string &_appid, const string &_id) :
    qid(_id), appid(_appid), msg_to_msec(0), time_wait_msec(0), monitors_cnt(0),
    qstat(MQ_MODULE_STATITEM_KEYRANGE, &__mq_sm)
{
    INIT_LIST_HEAD(&monitors);
    INIT_LIST_LINK(&mqp_node);
    __mq_sm.setup(_appid, _id);
    memset(&tw_timeout_node, 0, sizeof(tw_timeout_node));
    msgqueue_mem_stats->alloc++;
    msgqueue_mem_stats->alloc_size += sizeof(MQueue);
}

MQueue::~MQueue() {
    struct kmqmsg *req = NULL;
    while ((req = Pop()) != NULL) {
	mem_free(req, MSGPKGLEN(req) + RTLEN);
    }
    msgqueue_mem_stats->alloc--;
    msgqueue_mem_stats->alloc_size -= sizeof(MQueue);
}

int MQueue::MonitorsNum() {
    return monitors_cnt;
}
    
int MQueue::AddMonitor(msg_queue_monitor *monitor) {
    if (monitor->__attach_to_mqmonitor_head(&monitors) == 0) {
	if (Size() == 0)
	    monitor->mq_empty_callback_func();
	else if (Size() > 0)
	    monitor->mq_nonempty_callback_func();
	monitors_cnt++;
    }
    return 0;
}
    
int MQueue::DelMonitor(msg_queue_monitor *monitor) {
    if (monitor->__detach_from_mqmonitor_head() == 0)
	monitors_cnt--;
    return 0;
}
    
int MQueue::Setup(int qcap, int tw_msec, int to_msec)
{
    qstat.setkey(CAP, qcap);
    msg_to_msec = to_msec;
    time_wait_msec = tw_msec;
    return Init(qcap);
}

int MQueue::PushMsg(struct kmqmsg *msg) {
    struct kmqmsg *tmp = NULL;
    struct list_link *pos = NULL;
    char hlog[HEADER_BUFFERLEN] = {};

    while (-1 == Push(msg)) {
	if ((tmp = Pop()) != NULL) {
	    header_parse(&msg->hdr, hlog);
	    mem_free(msg, MSGPKGLEN(msg) + RTLEN);
	}
	KMQLOG_NOTICE("%s %s(d) drop %s of full queue", cappid(), cid(), hlog);
    }
    qstat.setkey(SIZE, Size());
    if (Size() == 1) {
	list_for_each_list_link(pos, &monitors) {
	    msg_queue_monitor *monitor = list_monitor(pos);
	    monitor->mq_nonempty_callback_func();
	}
    }
    return 0;
}


int MQueue::attach_to_mqpool_head(struct list_head *head) {
    if (!mqp_node.linked) {
	mqp_node.linked = 1;
	list_add(&mqp_node.node, head);
	return 0;
    }
    return -1;
}

int MQueue::detach_from_mqpool_head() {
    if (mqp_node.linked) {
	list_del(&mqp_node.node);
	mqp_node.linked = 0;
	return 0;
    }
    return -1;
}


static int __compute_mq_status(module_stat *qstat, int ts) {
    qstat->incrkey(PASSED, 1);
    if (ts > qstat->getkey(MAX_MSEC) || qstat->getkey(MAX_MSEC) == 0)
	qstat->setkey(MAX_MSEC, ts);
    if (ts < qstat->getkey(MIN_MSEC) || qstat->getkey(MIN_MSEC) == 0)
	qstat->setkey(MIN_MSEC, ts);
    qstat->incrkey(ALL_MSEC, ts);
    qstat->setkey(AVG_MSEC, qstat->getkey(ALL_MSEC) / qstat->getkey(PASSED));
    return 0;
}

static int __compute_mq_to_status(module_stat *qstat, int ts) {
    qstat->incrkey(DROPPED, 1);
    if (ts > qstat->getkey(MAX_TO_MSEC) || qstat->getkey(MAX_TO_MSEC) == 0)
	qstat->setkey(MAX_TO_MSEC, ts);
    if (ts < qstat->getkey(MIN_TO_MSEC) || qstat->getkey(MIN_TO_MSEC) == 0)
	qstat->setkey(MIN_TO_MSEC, ts);
    qstat->incrkey(ALL_TO_MSEC, ts);
    qstat->setkey(AVG_TO_MSEC, qstat->getkey(ALL_TO_MSEC) / qstat->getkey(DROPPED));
    return 0;
}



bool MQueue::msg_is_timeout(struct kmqmsg *msg) {
    char hlog[HEADER_BUFFERLEN] = {};
    int64_t to_msec = rt_mstime() - msg->hdr.timestamp;
    
    if (msg_to_msec && to_msec > msg_to_msec) {
	__compute_mq_to_status(&qstat, to_msec);
	header_parse(&msg->hdr, hlog);
	KMQLOG_WARN("%s %s(q) drop (%ldms max:%d) msg %s", cappid(), cid(),
		      to_msec, msg_to_msec, hlog);
	mem_free(msg, MSGPKGLEN(msg) + RTLEN);
	return true;
    }
    __compute_mq_status(&qstat, to_msec);
    return false;
}



struct kmqmsg *MQueue::PopMsg() {
    struct list_link *pos = NULL;
    struct kmqmsg *msg = NULL;

    while ((msg = Pop()) != NULL && msg_is_timeout(msg)) {
	// reach here only when msg is timeout
    }
    if (Size() == 0) {
	list_for_each_list_link(pos, &monitors) {
	    msg_queue_monitor *monitor = list_monitor(pos);
	    monitor->mq_empty_callback_func();
	}
    }
    qstat.setkey(SIZE, Size());
    return msg;
}



}
