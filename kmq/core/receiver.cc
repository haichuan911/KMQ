#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "receiver.h"
#include "mem_status.h"
#include "mqp.h"
#include "icmpmsg.h"

KMQ_DECLARATION_START


extern kmq_mem_status_t kmq_mem_stats;

static mem_stat_t *receivers_mem_stats = &kmq_mem_stats.receivers;


Receiver::Receiver(const string &_appid, const string &roleid) :
    Role(_appid, roleid, ROLE_RECEIVER)
{
    receivers_mem_stats->alloc++;
    receivers_mem_stats->alloc_size += sizeof(Receiver);
}


Receiver::Receiver(int rt, const string &_appid, const string &roleid) :
    Role(_appid, roleid, rt)
{
    receivers_mem_stats->alloc++;
    receivers_mem_stats->alloc_size += sizeof(Receiver);
}

Receiver::~Receiver() {
    Conn *conn = NULL;
    unBind(&conn);
    if (conn)
	delete conn;
    receivers_mem_stats->alloc--;
    receivers_mem_stats->alloc_size -= sizeof(Receiver);
}

int Receiver::process_error() {
    module_stat *rstat = Stat();

    pp._reset_recv();
    SET_MSGERROR(this);
    if (errno == KMQ_EPACKAGE)
	rstat->incrkey(CHECKSUM_ERRORS);
    rstat->incrkey(RECV_ERRORS);
    return 0;
}

    
int Receiver::process_normal_massage(struct kmqmsg *msg) {
    uint32_t size = 0;
    struct kmqrt *rt = NULL;
    module_stat *rstat = Stat();

    size = msg->hdr.size;
    msg->route = (struct kmqrt *)(msg->data + size - RTLEN);
    rt = msg->route;
    pp._reset_recv();
    rstat->incrkey(TRANSFERTIME, (uint32_t)rt_mstime() - rt->u.env.sendstamp);
    rstat->incrkey(RECV_PACKAGES);
    rstat->incrkey(RECV_BYTES, MSGPKGLEN(msg));
    return 0;
}

int Receiver::process_self_icmp(struct kmqmsg *msg) {

    return -1;
}

int Receiver::Recv(struct kmqmsg **req) {

    int ret = 0;
    struct kmqmsg *nmsg = NULL;
    Conn *internconn = Connect();

    if ((ret = pp._recv_massage(internconn, &nmsg, RTLEN)) == 0) {
	if (IS_ICMP(nmsg)) {
	    if (process_self_icmp(nmsg) < 0)
		PushRCVICMP(nmsg);
	    return -1;
	}
	process_normal_massage(nmsg);
	*req = nmsg;
    } else if (ret < 0 && errno != EAGAIN)
	process_error();

    return ret;
}

int Receiver::Send(struct kmqmsg *msg) {
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


int Receiver::BatchSend(int max_send) {
    MQueue *mq = Queue();
    struct kmqmsg *resp = NULL;
    char hlog[HEADER_BUFFERLEN] = {};
    int ret = 0, idx;

    if (!mq) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (max_send <= 0)
	max_send = mq->Size();

    send_icmp();

    for (idx = 0; idx < max_send; idx++) {
	if (!(resp = mq->PopMsg()))
	    break;
	if ((ret = Send(resp)) < 0) {
	    header_parse(&resp->hdr, hlog);
	    mem_free(resp, MSGPKGLEN(resp) + RTLEN);
	    KMQLOG_ERROR("%s [r:%s:%s] send response %s with errno %d",
			   cappid(), cid(), cip(), hlog, errno);
	    break;
	}
	mem_free(resp, MSGPKGLEN(resp) + RTLEN);
	resp = NULL;
    }
    return idx;
}

int Receiver::send_icmp() {
    Conn *internconn = Connect();
    struct kmqhdr *hdr = NULL;
    struct kmqmsg *icmp_msg = NULL;

    while ((icmp_msg = PopSNDICMP()) != NULL) {
	hdr = &icmp_msg->hdr;
	if (Send(icmp_msg) < 0) {
	    KMQLOG_ERROR("%s %s(r) send icmp %d with errno %d",
			   cappid(), cid(), hdr->flags, errno);
	    mem_free(icmp_msg, MSGPKGLEN(icmp_msg));
	    break;
	}
	KMQLOG_INFO("%s %s(r) send icmp %d", cappid(), cid(), hdr->flags);
	mem_free(icmp_msg, MSGPKGLEN(icmp_msg));
    }
    while (internconn->Flush() < 0 && errno == EAGAIN) {
	/* flush cache */
    }
    return 0;
}

}
