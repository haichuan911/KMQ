#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>
#include "memalloc.h"
#include "mux.h"


using namespace std;

KMQ_DECLARATION_START


MuxProducer *NewMuxProducer() {
    __SpioInternMuxProducer *mux = new (std::nothrow) __SpioInternMuxProducer();
    return mux;
}


__SpioInternMuxProducer::__SpioInternMuxProducer() :
    inited(0), sndwin(0), congestion_window(1), _stopping(true),
    poller(NULL),
    stats(MUX_MODULE_STATITEM_KEYRANGE, NULL), empty_retries(0)
{
    memset(&env, 0, sizeof(env));
    poller = EpollCreate(1024, 100);
}

static int muxc_req_cleanup_func(struct appmsg *msg) {
    if (!msg) {
	errno = EINVAL;
	return -1;
    }
    mem_free(msg, sizeof(*msg) + msg->s.len);
    return 0;
}

static int muxc_resp_cleanup_func(struct appmsg *msg) {
    if (!msg) {
	errno = EINVAL;
	return -1;
    }
    if (msg->s.len)
	mem_free(msg->s.data, msg->s.len);
    mem_free(msg, sizeof(*msg));
    return 0;
}

    
__SpioInternMuxProducer::~__SpioInternMuxProducer() {
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

inline int __SpioInternMuxProducer::disable_event(int ev) {
    if (!poller)
	return -1;
    if (ee.events & ev) {
	ee.events &= ~ev;
	return poller->CtlMod(&ee);
    }
    return 0;
}


inline int __SpioInternMuxProducer::enable_event(int ev) {
    if (!poller)
	return -1;
    ee.events |= ev;
    return poller->CtlMod(&ee);
}


inline int __SpioInternMuxProducer::pop_reqs(struct list_head *head, int size) {
    int i = 0, cnt = 0;
    struct appmsg *msg = NULL;

    req_queue.Lock();
    for (i = 0; i < size; i++) {
	if (!(msg = req_queue.Pop()))
	    break;
	cnt++;
	list_add_tail(&msg->node, head);
    }
    if (req_queue.Empty()) {
	if (empty_retries > 2) {
	    empty_retries = 0;
	    disable_event(EPOLLOUT);
	} else
	    empty_retries++;
    }
    req_queue.UnLock();
    return cnt;
}

inline int __SpioInternMuxProducer::push_req(struct appmsg *msg) {
    struct appmsg *tmp = NULL;

    if (!msg) {
	errno = EINVAL;
	return -1;
    }
    req_queue.Lock();
    if (req_queue.Empty())
	enable_event(EPOLLOUT);
    while (req_queue.Push(msg) == -1) {
	if ((tmp = req_queue.Pop()) != NULL)
	    muxc_req_cleanup_func(tmp);
    }
    req_queue.UnLock();
    return 0;
}


inline struct appmsg *__SpioInternMuxProducer::pop_resp() {
    struct appmsg *msg = NULL;

    resp_queue.Lock();
    if (!(msg = resp_queue.Pop()))
	resp_queue.Wait();
    resp_queue.UnLock();
    return msg;
}


inline int __SpioInternMuxProducer::push_resps(struct list_head *head) {
    bool wakeup = false;
    struct appmsg *msg = NULL, *next = NULL, *tmp = NULL;

    if (!head) {
	errno = EINVAL;
	return -1;
    }
    resp_queue.Lock();
    if (resp_queue.Empty())
	wakeup = true;
    list_for_each_appmsg_safe(msg, next, head) {
	list_del(&msg->node);
	while (resp_queue.Push(msg) == -1) {
	    if ((tmp = resp_queue.Pop()) != NULL)
		muxc_resp_cleanup_func(tmp);
	}
    }
    if (wakeup)
	resp_queue.BroadCast();
    resp_queue.UnLock();
    return 0;
}

static int ioworker(void *arg_) {
    __SpioInternMuxProducer *mux = (__SpioInternMuxProducer *)arg_;
    return mux->intern_ioworker();
}

static int callbackworker(void *arg_) {
    __SpioInternMuxProducer *mux = (__SpioInternMuxProducer *)arg_;
    return mux->intern_callbackworker();
}


int __SpioInternMuxProducer::connect_error() {
    interncli.Close();
    poller->CtlDel(&ee);
    connect_to_kmqsvr();
    poller->CtlAdd(&ee);
    return 0;
}


int __SpioInternMuxProducer::connect_to_kmqsvr() {

    while (interncli.Connect(_appname, _apphost) < 0)
	usleep(10);
    ee.SetEvent(interncli.Fd(), EPOLLIN|EPOLLOUT|EPOLLRDHUP, this);
    stats.incrkey(RECONNECT);
    interncli.SetOption(OPT_SOCKCACHE);
    interncli.SetOption(OPT_NONBLOCK, 1);
    return 0;
}

    
int __SpioInternMuxProducer::recv_massage(int max_recv) {
    struct appmsg msg = {};
    int ret = 0, i;
    struct list_head head;

    INIT_LIST_HEAD(&head);
    for (i = 0; i < max_recv; i++) {
	if ((ret = interncli.RecvMsg(&msg)) == 0) {
	    struct appmsg *resp = NULL;
	    if ((resp = (struct appmsg *)mem_zalloc(sizeof(*resp)))) {
		resp->hdr = msg.hdr;
		resp->s.len = msg.s.len;
		resp->s.data = msg.s.data;
		list_add_tail(&resp->node, &head);
	    } else if (msg.s.len)
		mem_free(msg.s.data, msg.s.len);
	    stats.incrkey(RECVBYTES, msg.s.len);
	    stats.incrkey(RECVPACKAGES);
	    memset((char *)&msg, 0, sizeof(msg));
	} else if (ret < 0 && errno != EAGAIN) {
	    stats.incrkey(RECVERRORS);
	    connect_error();
	    break;
	}
    }
    if (!list_empty(&head))
	push_resps(&head);
    return ret;
}


static int __set_async_api_flags(struct kmqhdr *hdr) {
    hdr->flags = hdr->flags | ASYNC_FLAG;
    return 0;
}


int __SpioInternMuxProducer::send_massage(struct appmsg *req) {
    int ret = 0;
    uint32_t nbytes = 0;

    __set_async_api_flags(&req->hdr);
    nbytes = sizeof(*req) + req->s.len;
    ret = interncli.SendMsg(req);
    muxc_req_cleanup_func(req);
    if (ret == 0) {
	stats.incrkey(SENDBYTES, nbytes);
	stats.incrkey(SENDPACKAGES);
    } else if (ret < 0 && errno != EAGAIN) {
	stats.incrkey(SENDERRORS);
	connect_error();
    }
    interncli.FlushCache();
    return 0;
}



int __SpioInternMuxProducer::intern_ioworker() {
    int rcachesize = 0;
    struct appmsg *req = NULL, *tmp = NULL;
    struct list_head head;
    struct list_head io_head, to_head;

    INIT_LIST_HEAD(&head);
    while (!_stopping) {
	INIT_LIST_HEAD(&io_head);
	INIT_LIST_HEAD(&to_head);
	ee.happened = 0;
	if (poller->Wait(&io_head, &to_head, 1) < 0) {
	    usleep(10);
	    continue;
	}
	ht.cleanup_tw_callbacks();
	rcachesize = interncli.CacheSize();
	if (list_empty(&io_head) && rcachesize <= 0)
	    continue;
	if ((ee.happened & EPOLLIN) || rcachesize > 0)
	    recv_massage(1);
	if ((ee.happened & EPOLLOUT)) {
	    if (sndwin < congestion_window) {
		if (!list_empty(&head)) {
		    req = list_first_appmsg(&head);
		    list_del(&req->node);
		    send_massage(req);
		} else
		    pop_reqs(&head, congestion_window);
	    }
	}
	if (ee.happened & (EPOLLRDHUP|EPOLLERR))
	    connect_error();
	detach_for_each_poll_link(&io_head);
	detach_for_each_poll_link(&to_head);
    }
    
    list_for_each_appmsg_safe(req, tmp, &head) {
	muxc_req_cleanup_func(req);
    }
    
    return 0;
}

int __SpioInternMuxProducer::intern_callbackworker() {
    struct appmsg *resp = NULL;
    MpHandler *handler = NULL;

    while (!_stopping) {
	if ((resp = pop_resp()) == NULL) {
	    continue;
	}
	if ((handler = (MpHandler *)ht.FindHandler(resp->hdr.seqid)) != NULL)
	    handler->ServeKMQ(resp->s.data, resp->s.len);
	muxc_resp_cleanup_func(resp);
    }
    return 0;
}


int __SpioInternMuxProducer::Setup(const string &appname, const string &apphost,
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
    ht.Init(20000);
    req_queue.Setup(env.queue_cap, muxc_req_cleanup_func);
    resp_queue.Setup(env.queue_cap, muxc_resp_cleanup_func);
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

int __SpioInternMuxProducer::Stop() {
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
    resp_queue.BroadCast();
    for (it = mpthreads.begin(); it != mpthreads.end(); ++it) {
	thread = *it;
	thread->Stop();
    }
    return 0;
}

int __SpioInternMuxProducer::StartServe() {
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

int __SpioInternMuxProducer::SendRequest(const char *data, int len, MpHandler *handler, int to_msec) {
    struct appmsg *msg = NULL;
    int64_t hid = 0, nbytes = 0;

    if (!poller || _stopping) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (len < 0) {
	errno = EINVAL;
	return -1;
    }
    nbytes = sizeof(*msg) + len;
    if ((msg = (struct appmsg *)mem_zalloc(nbytes)) == NULL) {
	errno = ENOMEM;
	return -1;
    }
    hid = ht.GetHid();
    ht.InsertHandler(hid, handler, to_msec);
    msg->s.len = len;
    msg->s.data = (char *)msg + sizeof(*msg);
    msg->hdr.seqid = hid;
    if (len > 0)
	memcpy(msg->s.data, data, len);
    push_req(msg);
    return 0;
}

}
