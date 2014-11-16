#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>
#include "os.h"
#include "memalloc.h"
#include "mux.h"



KMQ_DECLARATION_START


static int ioworker(void *arg_) {
    __SpioInternMuxComsumer *mux = (__SpioInternMuxComsumer *)arg_;
    return mux->intern_ioworker();
}

static int callbackworker(void *arg_) {
    __SpioInternMuxComsumer *mux = (__SpioInternMuxComsumer *)arg_;
    return mux->intern_callbackworker();
}

static int muxs_waitqueue_cleanup_func(struct appmsg *msg) {
    if (msg->s.len)
	mem_free(msg->s.data, msg->s.len);
    if (msg->rt)
	mem_free(msg->rt, appmsg_rtlen(msg->rt->ttl));
    mem_free(msg, sizeof(*msg));
    return 0;
}
    
MuxComsumer *NewMuxComsumer() {
    __SpioInternMuxComsumer *mux = new (std::nothrow) __SpioInternMuxComsumer();
    return mux;
}

__SpioInternMuxComsumer::__SpioInternMuxComsumer() :
    inited(0), _stopping(true), _handler(NULL),
    poller(NULL), stats(MUX_MODULE_STATITEM_KEYRANGE, NULL)
{
    memset(&env, 0, sizeof(env));
    poller = EpollCreate(1024, 100);
}

__SpioInternMuxComsumer::~__SpioInternMuxComsumer() {
    OSThread *thread = NULL;
    list<OSThread *>::iterator it;

    if (poller) {
	poller->CtlDel(&ee);
	delete poller;
    }
    for (it = mpthreads.begin(); it != mpthreads.end(); ++it) {
	thread = *it;
	delete thread;
    }
}

inline int __SpioInternMuxComsumer::disable_event(int ev) {
    if (!poller)
	return -1;
    if (ee.events & ev) {
	ee.events &= ~ev;
	return poller->CtlMod(&ee);
    }
    return 0;
}


inline int __SpioInternMuxComsumer::enable_event(int ev) {
    if (!poller)
	return -1;
    ee.events |= ev;
    return poller->CtlMod(&ee);
}

inline struct appmsg *__SpioInternMuxComsumer::pop_req() {
    struct appmsg *msg = NULL;

    req_queue.Lock();
    if (!(msg = req_queue.Pop()))
	req_queue.Wait();
    req_queue.UnLock();
    return msg;
}


inline int __SpioInternMuxComsumer::push_req(struct appmsg *msg) {
    bool wakeup = false;
    struct appmsg *tmp = NULL;

    req_queue.Lock();
    if (req_queue.Empty())
	wakeup = true;
    while (req_queue.Push(msg) == -1) {
	if ((tmp = req_queue.Pop()) != NULL) {
	    _handler->DropMsg(tmp->s.data, tmp->s.len, DROPREQ_QUEUEFULL);
	    muxs_waitqueue_cleanup_func(tmp);
	}
    }
    if (wakeup)
	req_queue.BroadCast();
    req_queue.UnLock();
    return 0;
}



inline struct appmsg *__SpioInternMuxComsumer::pop_resp() {
    struct appmsg *msg = NULL;

    resp_queue.Lock();
    msg = resp_queue.Pop();
    if (resp_queue.Empty())
	disable_event(EPOLLOUT);
    resp_queue.UnLock();
    return msg;
}


inline int __SpioInternMuxComsumer::push_resp(struct appmsg *resp) {
    struct appmsg *tmp = NULL;

    resp_queue.Lock();
    if (resp_queue.Empty())
	enable_event(EPOLLOUT);
    while (resp_queue.Push(resp) == -1) {
	if ((tmp = resp_queue.Pop()) != NULL) {
	    _handler->DropMsg(tmp->s.data, tmp->s.len, DROPRESP_QUEUEFULL);
	    muxs_waitqueue_cleanup_func(tmp);
	}
    }
    resp_queue.UnLock();
    return 0;
}

int __SpioInternMuxComsumer::Setup(const string &appname, const string &apphost,
				   const struct mux_conf *arg) {
    int i = 0;
    OSThread *thread = NULL;
    struct mux_conf default_arg = {
	1,
	20000,
	0,
    };

    if (inited || !poller) {
	errno = KMQ_EINTERN;
	return -1;
    }
    
    _appname = appname;
    _apphost = apphost;
    if (!arg)
	arg = &default_arg;
    env = *arg;
    req_queue.Setup(env.queue_cap, muxs_waitqueue_cleanup_func);
    resp_queue.Setup(env.queue_cap, muxs_waitqueue_cleanup_func);
    connect_to_kmqsvr();
    poller->CtlAdd(&ee);
    if (env.callback_workers <= 0)
	env.callback_workers = 1;
    for (i = 0; i < env.callback_workers; i++) {
	if ((thread = new (std::nothrow) OSThread()) == NULL) {
	    errno = ENOMEM;
	    break;
	}
	mpthreads.push_back(thread);
    }
    inited = 1;
    return 0;
}


int __SpioInternMuxComsumer::SetHandler(McHandler *handler) {

    if (!poller || _handler) {
	errno = KMQ_EINTERN;
	return -1;
    }
    _handler = handler;
    return 0;
}


int __SpioInternMuxComsumer::connect_error() {
    internsvr.Close();
    poller->CtlDel(&ee);
    connect_to_kmqsvr();
    poller->CtlAdd(&ee);
    return 0;
}


int __SpioInternMuxComsumer::connect_to_kmqsvr() {

    while (internsvr.Connect(_appname, _apphost) < 0)
	usleep(10);
    ee.SetEvent(internsvr.Fd(), EPOLLIN|EPOLLOUT|EPOLLRDHUP, this);
    stats.incrkey(RECONNECT);
    internsvr.SetOption(OPT_SOCKCACHE);
    internsvr.SetOption(OPT_NONBLOCK, 1);
    return 0;
}


int __SpioInternMuxComsumer::recv_massage(int max_recv) {
    int ret = 0, cnt = 0;
    bool istimeout = false;
    struct appmsg pkg = {}, *req = NULL;

    while (max_recv > 0) {
	if ((ret = internsvr.RecvMsg(&pkg)) < 0)
	    break;
	if (!(istimeout = __package_is_timeout(&pkg, env.max_trip_time))
	    && ((req = (struct appmsg *)mem_zalloc(sizeof(*req))) != NULL)) {
	    memcpy(req, (char *)&pkg, sizeof(pkg));
	    push_req(req);
	} else if (pkg.s.len) {
	    if (istimeout)
		_handler->DropMsg(pkg.s.data, pkg.s.len, DROPREQ_TIMEOUT);
	    mem_free(pkg.rt);
	    mem_free(pkg.s.data, pkg.s.len);
	}
	max_recv--;
	cnt++;
	stats.incrkey(RECVPACKAGES);
	stats.incrkey(RECVBYTES, pkg.s.len);
	memset((char *)&pkg, 0, sizeof(pkg));
    }
    if (ret < 0 && errno != EAGAIN) {
	stats.incrkey(RECVERRORS);
	connect_error();
    }
    return cnt;
}

int __SpioInternMuxComsumer::send_massage(int max_send) {
    int ret = 0, cnt = 0;
    uint32_t snd_bytes = 0;
    struct appmsg *resp = NULL;

    while (max_send > 0 && (resp = pop_resp()) != NULL) {
	cnt++;
	max_send--;
	snd_bytes = resp->s.len + appmsg_rtlen(resp->rt->ttl);
	ret = internsvr.SendMsg(resp);
	muxs_waitqueue_cleanup_func(resp);
	if (ret == 0) {
	    stats.incrkey(SENDBYTES, snd_bytes);
	    stats.incrkey(SENDPACKAGES);
	} else if (ret < 0 && errno != EAGAIN) {
	    stats.incrkey(SENDERRORS);
	    connect_error();
	}
    }
    if (cnt > 0)
	internsvr.FlushCache();
    return 0;
}


int __SpioInternMuxComsumer::intern_ioworker() {
    int rcachesize = 0;
    struct list_head io_head, to_head;

    while (!_stopping) {

	INIT_LIST_HEAD(&io_head);
	INIT_LIST_HEAD(&to_head);
	ee.happened = 0;

	if (poller->Wait(&io_head, &to_head, 1) < 0) {
	    usleep(10);
	    continue;
	}
	rcachesize = internsvr.CacheSize();
	if (list_empty(&io_head) && rcachesize <= 0)
	    continue;
	if ((ee.happened & EPOLLIN) || rcachesize > 0)
	    recv_massage(env.callback_workers);
	if (ee.happened & EPOLLOUT)
	    send_massage(env.callback_workers);
	if (ee.happened & (EPOLLRDHUP|EPOLLERR))
	    connect_error();

	detach_for_each_poll_link(&io_head);
	detach_for_each_poll_link(&to_head);
    }

    return 0;
}



int __SpioInternMuxComsumer::intern_callbackworker() {
    struct appmsg *req = NULL;
    SpioReadWriter rw;

    while (!_stopping) {
	if ((req = pop_req()) == NULL) {
	    continue;
	}
	rw.req = req;
	rw.mux_comsumer = this;
	_handler->ServeKMQ(rw, rw);
	muxs_waitqueue_cleanup_func(req);
    }
    return 0;
}

int __SpioInternMuxComsumer::Stop() {
    OSThread *thread = NULL;
    list<OSThread *>::iterator it;

    if (!poller) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (_stopping) {
	errno = KMQ_EDUPOP;
	return -1;
    }
    _stopping = true;
    iothread.Stop();
    req_queue.BroadCast();
    for (it = mpthreads.begin(); it != mpthreads.end(); ++it) {
	thread = *it;
	thread->Stop();
    }
    return 0;
}


int __SpioInternMuxComsumer::StartServe() {
    OSThread *thread = NULL;
    list<OSThread *>::iterator it;

    if (!poller) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!_stopping) {
	errno = KMQ_EDUPOP;
	return -1;
    }
    _stopping = false;
    for (it = mpthreads.begin(); it != mpthreads.end(); ++it) {
	thread = *it;
	thread->Start(callbackworker, this);
    }
    iothread.Start(ioworker, this);
    return 0;
}

int __SpioInternMuxComsumer::SendResponse(const char *data, int len, const string &rt) {
    struct kmqhdr *hdr = NULL;
    struct appmsg *resp = NULL;
    
    if (len < 0) {
	errno = EINVAL;
	return -1;
    }
    hdr = (struct kmqhdr *)rt.data();
    if (!(resp = (struct appmsg *)mem_zalloc(sizeof(*resp)))) {
	errno = ENOMEM;
	return -1;
    }
    resp->hdr = *hdr;
    if (!(resp->rt = (struct appmsg_rt *)mem_zalloc(appmsg_rtlen(hdr->ttl)))) {
	mem_free(resp, sizeof(*resp));
	errno = ENOMEM;
	return -1;
    }
    resp->s.len = len;
    if (resp->s.len && !(resp->s.data = (char *)mem_zalloc(resp->s.len))) {
	mem_free(resp->rt, appmsg_rtlen(hdr->ttl));
	mem_free(resp, sizeof(*resp));
	errno = ENOMEM;
	return -1;
    }
    memcpy(resp->s.data, data, len);
    memcpy(resp->rt, rt.data() + sizeof(resp->hdr), appmsg_rtlen(hdr->ttl));
    push_resp(resp);
    return 0;
}




}
