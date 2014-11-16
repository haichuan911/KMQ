#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "memalloc.h"
#include "appctx.h"

KMQ_DECLARATION_START


int AppCtx::receiver_recv(Role *r) {
    struct kmqmsg *req = NULL;
    MQueue *mq = NULL;
    Role *d = NULL;
    int rn = 0, msg_balance_cnt = IS_PIORECEIVER(r->Type()) ? conf.msg_balance_factor : 10;

    while (msg_balance_cnt > 0) {
	if (!req && r->Recv(&req) < 0)
	    break;
	msg_balance_cnt--;
	if (!(d = lbp->loadbalance_send(NULL))) {
	    continue;
	}
	mq = d->Queue();
	mq->PushMsg(req);
	req = NULL;
	rn++;
    }
    if (req) {
	KMQLOG_ERROR("%s %s(r) drop request with no dispatchers %d", cid(), r->cid(), lbp->size());
	mem_free(req, MSGPKGLEN(req) + RTLEN);
    }
    if (rn > 0) {
	astat.incrkey(RRCVPKG, rn);
	KMQLOG_INFO("%s %s(r) total recv %d request", cid(), r->cid(), rn);
    }

    return 0;
}
    

int AppCtx::dispatcher_recv(Role *r) {
    struct kmqmsg *resp = NULL;
    string rid;
    MQueue *mq = NULL;
    int rn = 0, msg_balance_cnt = IS_PIODISPATCHER(r->Type()) ? conf.msg_balance_factor : 10;

    mq = r->Queue();
    if (HAS_MSGERROR(r))
	msg_balance_cnt = mq->Cap();
    while (msg_balance_cnt > 0) {
	if (r->Recv(&resp))
	    break;
	msg_balance_cnt--;
	route_parse(resp->route, rid);
	if ((mq = mqp.Find(rid))) {
	    mq->PushMsg(resp);
	} else {
	    mem_free(resp, MSGPKGLEN(resp) + RTLEN);
	    KMQLOG_ERROR("%s found %s(q) doesn't exist", cid(), rid.c_str());
	}
	rn++;
    }
    if (rn > 0) {
	astat.incrkey(DRCVPKG, rn);
	KMQLOG_INFO("%s %s(d) total recv %d response", cid(), r->cid(), rn);
    }

    return 0;
}


int AppCtx::receiver_send(Role *r) {
    Conn *internconn = NULL;
    int ret = 0, sn = 0, msg_balance_cnt = -1;

    if ((sn = r->BatchSend(msg_balance_cnt)) > 0) {
	KMQLOG_INFO("%s %s(r) total send %d response", cid(), r->cid(), sn);
	internconn = r->Connect();
	while ((ret = internconn->Flush()) < 0 && errno == EAGAIN) {
	    /* flush cache */
	}
	if (ret < 0 && errno != EAGAIN)
	    KMQLOG_ERROR("%s %s(r) flush cache errno(%d)", cid(), r->cid(), errno);
    }
    if (sn > 0)
	astat.incrkey(RSNDPKG, sn);
    return 0;
}

int AppCtx::dispatcher_send(Role *r) {
    Conn *internconn = NULL;
    int ret = 0, sn = 0, msg_balance_cnt = -1;

    if ((sn = r->BatchSend(msg_balance_cnt)) > 0) {
	KMQLOG_INFO("%s %s(d) total send %d request", cid(), r->cid(), sn);
	internconn = r->Connect();
	while ((ret = internconn->Flush()) < 0 && errno == EAGAIN) {
	    /* flushing cache */
	}
	if (ret < 0 && errno != EAGAIN)
	    KMQLOG_ERROR("%s %s(d) flush cache with errno %d", cid(), r->cid(), errno);
    }
    if (sn > 0)
	astat.incrkey(DSNDPKG, sn);
    return 0;
}


int AppCtx::recv_massage(struct list_head *rhead, struct list_head *dhead) {
    Role *r = NULL;
    struct list_link *pos = NULL, *next = NULL;

    if (rhead && !list_empty(rhead)) {
	list_for_each_list_link_safe(pos, next, rhead) {
	    r = list_r(pos);
	    CLEAR_MSGIN(r);
	    r->detach_from_msgin_head();
	    receiver_recv(r);
	    if (HAS_MSGERROR(r)) {
		// if error happend. don't send any data into network
		CLEAR_MSGOUT(r);
		r->detach_from_msgout_head();
		r->attach_to_msgerror_head(&err_roles);
	    }
	    astat.incrkey(POLLRIN);
	}
    }
    if (dhead && !list_empty(dhead)) {
	list_for_each_list_link_safe(pos, next, dhead) {
	    r = list_r(pos);
	    CLEAR_MSGIN(r);
	    r->detach_from_msgin_head();
	    dispatcher_recv(r);
	    if (HAS_MSGERROR(r)) {
		// if error happend. don't send any data into network
		CLEAR_MSGOUT(r);
		r->detach_from_msgout_head();
		r->attach_to_msgerror_head(&err_roles);
	    }
	    astat.incrkey(POLLDIN);
	}
    }
    return 0;
}


int AppCtx::sendto_massage(struct list_head *rhead, struct list_head *dhead) {
    Role *r = NULL;
    struct list_link *pos = NULL, *next = NULL;

    // First. process receivers MSGOUT events, that's send response
    if (rhead && !list_empty(rhead)) {
	list_for_each_list_link_safe(pos, next, rhead) {
	    r = list_r(pos);
	    CLEAR_MSGOUT(r);
	    r->resource_stat_module_timestamp_update(now_time);
	    r->detach_from_msgout_head();
	    receiver_send(r);
	    if (HAS_MSGERROR(r))
		r->attach_to_msgerror_head(&err_roles);
	    astat.incrkey(POLLROUT);
	}
    }

    // Second. process dispatchers MSGOUT events, that's send request
    if (dhead && !list_empty(dhead)) {
	list_for_each_list_link_safe(pos, next, dhead) {
	    r = list_r(pos);
	    CLEAR_MSGOUT(r);
	    r->resource_stat_module_timestamp_update(now_time);
	    r->detach_from_msgout_head();
	    dispatcher_send(r);
	    if (HAS_MSGERROR(r))
		r->attach_to_msgerror_head(&err_roles);
	    astat.incrkey(POLLDOUT);
	}
    }

    return 0;
}

static int mark_msgin_when_role_has_cache(Role *r, void *data) {
    Conn *internconn = NULL;
    struct list_head *head = (struct list_head *)data;

    if (HAS_MSGIN(r))
	return 0;
    internconn = r->Connect();
    if (internconn->CacheSize(SO_READCACHE) > 0) {
	SET_MSGIN(r);
	r->attach_to_msgin_head(head);
    }
    return 0;
}
    


int AppCtx::wait_massage(struct list_head *rin, struct list_head *rout,
			 struct list_head *din, struct list_head *dout) {
    EpollEvent *ev = NULL;
    Role *r = NULL;
    struct list_head io_head, to_head;
    struct list_link *pos = NULL, *next = NULL;
    
    INIT_LIST_HEAD(&io_head);
    INIT_LIST_HEAD(&to_head);

    if (poller->Wait(&io_head, &to_head, conf.epoll_timeout_msec) < 0) {
	KMQLOG_ERROR("poller wait: %s", kmq_strerror(errno));
	return -1;
    }
    list_for_each_list_link_safe(pos, next, &to_head) {
	ev = list_ev(pos);
	ev->detach();
	r = ROLEOFEV(ev);
	r->attach_to_msgerror_head(&err_roles);
    }
    list_for_each_list_link_safe(pos, next, &io_head) {
	ev = list_ev(pos);
	ev->detach();
	r = ROLEOFEV(ev);
	if (IS_RECEIVER(r->Type()))
	    r->AttachToMsgevHead(rin, rout, &err_roles);
	else if (IS_DISPATCHER(r->Type()))
	    r->AttachToMsgevHead(din, dout, &err_roles);
    }
    rom.WalkReceivers(mark_msgin_when_role_has_cache, rin);
    rom.WalkDispatchers(mark_msgin_when_role_has_cache, din);
    detach_for_each_poll_link(&io_head);
    detach_for_each_poll_link(&to_head);

    return 0;
}



int AppCtx::process_massage() {
    struct list_head receiver_ihead = {}, dispatcher_ihead = {};
    struct list_head receiver_ohead = {}, dispatcher_ohead = {};

    INIT_LIST_HEAD(&receiver_ihead);
    INIT_LIST_HEAD(&receiver_ohead);
    INIT_LIST_HEAD(&dispatcher_ihead);
    INIT_LIST_HEAD(&dispatcher_ohead);

    lbp->balance();

    wait_massage(&receiver_ihead, &receiver_ohead,
		 &dispatcher_ihead, &dispatcher_ohead);

    // Process Msgev IN/OUT first and then ERROR. !importance
    // EPOLLRDHUP maybe happend with EPOLLIN. so we should recv
    // all data from cache and then process EPOLLRDHUP error.
    recv_massage(&receiver_ihead, &dispatcher_ihead);
    sendto_massage(&receiver_ohead, &dispatcher_ohead);

    if (!list_empty(&receiver_ihead) || !list_empty(&receiver_ohead))
	KMQLOG_ERROR("impossible reach here: %d", errno);
    if (!list_empty(&dispatcher_ihead) || !list_empty(&dispatcher_ohead))
	KMQLOG_ERROR("impossible reach here: %d", errno);

    if (!list_empty(&err_roles))
	process_roles_error();
    return 0;
}



}    
