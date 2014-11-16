#ifndef _H_KMQMUX_
#define _H_KMQMUX_


#include <kmq/kmq.h>


namespace kmq {

enum DROPMSG_EV {
    DROPREQ_QUEUEFULL = 1,
    DROPREQ_TIMEOUT = 2,
    DROPRESP_QUEUEFULL = 3,
};

struct mux_conf {
    int callback_workers;
    int queue_cap;
    int max_trip_time;
};

class ReqReader {
 public:
    virtual ~ReqReader() {}
    virtual const char *Data() = 0;
    virtual uint32_t Len() = 0;
    virtual string Route() = 0;
};

class ResponseWriter {
 public:
    virtual ~ResponseWriter() {}
    virtual int Send(const char *data, uint32_t len, const string &rt) = 0;
};

class McHandler {
 public:
    virtual ~McHandler() {}
    virtual int DropMsg(const char *data, int len, int ev) = 0;
    virtual int ServeKMQ(ReqReader &rr, ResponseWriter &rw) = 0;
};

class MuxComsumer {
 public:
    virtual ~MuxComsumer() {}
    virtual int Setup(const string &appname, const string &apphost,
		      const struct mux_conf *conf = NULL) = 0;
    virtual int SetHandler(McHandler *handler) = 0;
    virtual int Stop() = 0;
    virtual int StartServe() = 0;
    virtual int SendResponse(const char *data, int len, const string &rt) = 0;
};

MuxComsumer *NewMuxComsumer();



class MpHandler {
 public:
    virtual ~MpHandler() {}
    virtual int TimeOut(int to_msec) = 0;
    virtual int ServeKMQ(const char *data, int len) = 0;
};

class MuxProducer {
 public:
    virtual ~MuxProducer() {}
    virtual int Setup(const string &appname, const string &apphost,
		      const struct mux_conf *conf = NULL) = 0;
    virtual int Stop() = 0;
    virtual int StartServe() = 0;
    virtual int SendRequest(const char *data, int len, MpHandler *handler, int to_msec = 0) = 0;
};

MuxProducer *NewMuxProducer();


}










#endif  // _H_KMQMUX_
