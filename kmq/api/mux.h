#ifndef _H_KMQ_R_
#define _H_KMQ_R_

#include <list>
#include "os.h"
#include "list.h"
#include "ht.h"
#include "waitqueue.h"
#include "api.h"
#include "epoller.h"
#include "tmutex.h"
#include "pcond.h"
#include "osthread.h"
#include "module_stats.h"

KMQ_DECLARATION_START

enum {
    ASYNC_FLAG = FLAG_BIT5,
};

enum MUX_MODULE_STATITEM {
    RECONNECT = 1,
    RECVBYTES,
    SENDBYTES,
    RECVPACKAGES,
    SENDPACKAGES,
    RECVERRORS,
    SENDERRORS,
    CHECKSUMERRORS,
    MUX_MODULE_STATITEM_KEYRANGE,
};



class __SpioInternMuxComsumer;
class SpioReadWriter : public ReqReader, public ResponseWriter {
 public:
    SpioReadWriter();
    ~SpioReadWriter();
    friend class __SpioInternMuxComsumer;

    const char *Data();
    uint32_t Len();
    string Route();
    int Send(const char *data, uint32_t len, const string &rt);

 private:
    struct appmsg *req;
    __SpioInternMuxComsumer *mux_comsumer;
};

static inline int __package_is_timeout(struct appmsg *msg, uint64_t stt) {
    return stt > 0 && (rt_mstime() - msg->hdr.timestamp) > stt;
}

class __SpioInternMuxComsumer: public MuxComsumer {
public:
    __SpioInternMuxComsumer();
    ~__SpioInternMuxComsumer();
    friend class KMQReqReader;
    friend class KMQResponseWriter;

    int Setup(const string &appname, const string &apphost,
	      const struct mux_conf *conf = NULL);
    int SetHandler(McHandler *handler);
    int Stop();
    int StartServe();
    int SendResponse(const char *data, int len, const string &rt);
    
    int intern_ioworker();
    int intern_callbackworker();

private:
    int inited;
    bool _stopping;
    string _appname, _apphost;
    struct mux_conf env;
    McHandler *_handler;
    __SpioInternComsumer internsvr;
    EpollEvent ee;
    Epoller *poller;
    OSThread iothread;
    list<OSThread *> mpthreads;
    module_stat stats;
    WaitQueue req_queue, resp_queue;
    
    inline int disable_event(int ev);
    inline int enable_event(int ev);
    inline int recv_massage(int max_recv);
    inline int send_massage(int max_send);

    inline struct appmsg *pop_req();
    inline int push_req(struct appmsg *msg);
    inline struct appmsg *pop_resp();
    inline int push_resp(struct appmsg *resp);
    
    inline int connect_to_kmqsvr();
    inline int connect_error();
};






class __SpioInternMuxProducer : public MuxProducer {
public:
    __SpioInternMuxProducer();
    ~__SpioInternMuxProducer();

    int Setup(const string &appname, const string &apphost,
	      const struct mux_conf *conf = NULL);
    int Stop();
    int StartServe();
    int SendRequest(const char *data, int len, MpHandler *handler, int to_msec);

    int intern_ioworker();
    int intern_callbackworker();
    
private:
    int inited;
    int sndwin, congestion_window;   // kmq congestion window
    bool _stopping;
    string _appname, _apphost;
    struct mux_conf env;
    __SpioInternProducer interncli;
    EpollEvent ee;
    Epoller *poller;
    OSThread thread;
    OSThread iothread;
    list<OSThread *> mpthreads;
    module_stat stats;

    HandlerTable ht;    
    int empty_retries;
    WaitQueue req_queue, resp_queue;
    
    inline int disable_event(int ev);
    inline int enable_event(int ev);
    inline int send_massage(struct appmsg *req);
    inline int recv_massage(int max_recv);

    inline int pop_reqs(struct list_head *head, int size);
    inline int push_req(struct appmsg *req);
    inline struct appmsg *pop_resp();
    inline int push_resps(struct list_head *head);

    inline int connect_error();
    inline int connect_to_kmqsvr();
    inline int cleanup_tw_callbacks();
};










 

}



#endif  // _H_KMQ_R_
