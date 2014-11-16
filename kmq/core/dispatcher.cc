#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "os.h"
#include "mem_status.h"
#include "log.h"
#include "mqp.h"
#include "icmpmsg.h"
#include "dispatcher.h"


KMQ_DECLARATION_START

extern kmq_mem_status_t kmq_mem_stats;
static mem_stat_t *dispatchers_mem_stats = &kmq_mem_stats.dispatchers;

Dispatcher::Dispatcher(const string &_appid, const string &roleid) :
    Role(_appid, roleid, ROLE_DISPATCHER)
{
    dispatchers_mem_stats->alloc++;
    dispatchers_mem_stats->alloc_size += sizeof(Dispatcher);
}

Dispatcher::Dispatcher(int rt, const string &_appid, const string &roleid) :
    Role(_appid, roleid, rt)
{
    dispatchers_mem_stats->alloc++;
    dispatchers_mem_stats->alloc_size += sizeof(Dispatcher);
}


Dispatcher::~Dispatcher() {
    Conn *conn = NULL;
    unBind(&conn);
    if (conn)
	delete conn;
    dispatchers_mem_stats->alloc--;
    dispatchers_mem_stats->alloc_size -= sizeof(Dispatcher);
}

int Dispatcher::process_error() {
    module_stat *rstat = Stat();

    pp._reset_recv();
    SET_MSGERROR(this);
    if (errno == KMQ_EPACKAGE)
	rstat->incrkey(CHECKSUM_ERRORS);
    rstat->incrkey(RECV_ERRORS);
    return 0;
}


int Dispatcher::process_normal_massage(struct kmqmsg *msg) {
    struct kmqhdr *hdr = NULL;
    struct kmqrt *rt = NULL;
    module_stat *rstat = Stat();

    hdr = &msg->hdr;
    rt = (struct kmqrt *)(msg->data + hdr->size - RTLEN);
    rstat->incrkey(TRANSFERTIME, (uint32_t)rt_mstime() - rt->u.env.sendstamp);
    hdr->size -= RTLEN;
    hdr->ttl--;
    if (hdr->ttl)
	msg->route = (struct kmqrt *)(msg->data + hdr->size - RTLEN);
    else
	msg->route = NULL;
    pp._reset_recv();
    rstat->incrkey(RECV_PACKAGES);
    rstat->incrkey(RECV_BYTES, MSGPKGLEN(msg));
    return 0;
}

int Dispatcher::process_self_icmp(struct kmqmsg *icmp_msg) {
    struct kmqhdr *hdr = &icmp_msg->hdr;

    if (hdr->flags & ICMP_ROLESTATUS) {
	process_role_status_icmp(icmp_msg);
	return 0;
    }
    return -1;
}
    
    
int Dispatcher::Recv(struct kmqmsg **resp) {

    int ret = 0;
    struct kmqmsg *nmsg = NULL;
    Conn *internconn = Connect();

    if ((ret = pp._recv_massage(internconn, &nmsg, 0)) == 0) {
	if (IS_ICMP(nmsg)) {
	    if (process_self_icmp(nmsg) < 0)
		PushRCVICMP(nmsg);
	    return -1;
	}

	process_normal_massage(nmsg);
	*resp = nmsg;
    } else if (ret < 0 && errno != EAGAIN)
	process_error();

    return ret;
}


static void __append_route_info(struct kmqmsg *msg, const string rid) {
    struct kmqrt rt = {};

    route_unparse(rid, &rt);
    rt.u.env.sendstamp = (uint32_t)rt_mstime();

    // copy rt status into reserve space
    memcpy(msg->data + msg->hdr.size, (char *)&rt, RTLEN);
    msg->hdr.ttl++;
    msg->hdr.size += RTLEN;
}

int Dispatcher::Send(struct kmqmsg *msg) {
    int ret = 0;
    module_stat *rstat = Stat();
    Conn *internconn = Connect();

    if ((ret = pp._send_massage(internconn, msg)) == 0) {
	rstat->incrkey(SEND_PACKAGES);
	rstat->incrkey(SEND_BYTES, MSGPKGLEN(msg));
    } else if (ret < 0 && errno != EAGAIN) {
	pp._reset_send();
	SET_MSGERROR(this);
	if (errno == KMQ_EPACKAGE)
	    rstat->incrkey(CHECKSUM_ERRORS);
	rstat->incrkey(SEND_ERRORS);
    }
    return ret;
}

int Dispatcher::BatchSend(int max_send) {
    int ret = 0, idx = 0;
    MQueue *mq = Queue();
    char hlog[HEADER_BUFFERLEN] = {};
    struct kmqmsg *req = NULL, copyheader = {};
    
    if (!mq) {
	errno = KMQ_EINTERN;
	return -1;
    }
    send_icmp();
    if (!ROLECANW(this))
	return 0;
    if (max_send <= 0)
	max_send = mq->Size();
    while (mq->Size()) {
	if (!(req = mq->PopMsg()))
	    break;
	memcpy(&copyheader, req, sizeof(copyheader));
	__append_route_info(&copyheader, Id());
	if ((ret = Send(&copyheader)) < 0) {
	    header_parse(&req->hdr, hlog);
	    mem_free(req, MSGPKGLEN(req) + RTLEN);
	    KMQLOG_ERROR("%s [r:%s:%s] send request %s with errno %d",
			   cappid(), cid(), cip(), hlog, errno);
	    break;
	}
	idx++;
	mem_free(req, MSGPKGLEN(req) + RTLEN);
    }
    return idx;
}

int Dispatcher::send_icmp() {
    Conn *internconn = Connect();
    struct kmqhdr *hdr = NULL;
    struct kmqmsg *icmp_msg = NULL;

    while ((icmp_msg = PopSNDICMP()) != NULL) {
	if (Send(icmp_msg) < 0) {
	    hdr = &icmp_msg->hdr;
	    KMQLOG_ERROR("%s [d:%s:%s] send icmp %d with errno %d",
			   cappid(), cid(), cip(), hdr->flags, errno);
	    mem_free(icmp_msg, MSGPKGLEN(icmp_msg));
	    break;
	}
	mem_free(icmp_msg, MSGPKGLEN(icmp_msg));
    }
    while (internconn->Flush() < 0 && errno == EAGAIN) {
	/* flush cache */
    }
    return 0;
}

int Dispatcher::process_role_status_icmp(struct kmqmsg *msg) {
    MQueue *mq = Queue();
    struct role_status_icmp *other_peer = NULL;

    other_peer = (struct role_status_icmp *)msg->data;
    if (other_peer->dispatchers > 0 && !ROLECANW(this)) {
	EnablePOLLOUT();
	mq->AddMonitor(this);
	SETROLE_W(this);
	KMQLOG_WARN("%s [d:%s:%s] open write", cappid(), cid(), cip());
    } else if (other_peer->dispatchers <= 0 && ROLECANW(this)) {
	DisablePOLLOUT();
	mq->DelMonitor(this);
	detach_from_msgout_head();
	UNSETROLE_W(this);
	KMQLOG_WARN("%s [d:%s:%s] shutdown write", cappid(), cid(), cip());
    }
    mem_free(msg, MSGPKGLEN(msg));

    return 0;
}


}
