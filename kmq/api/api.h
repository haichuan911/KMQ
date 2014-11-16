#ifndef _H_KMQ_RAWAPI_
#define _H_KMQ_RAWAPI_

#include <kmq/kmq.h>
#include <kmq/kmqmux.h>
#include "net.h"
#include "proto.h"


KMQ_DECLARATION_START

#define KMQTEST

typedef struct _msghdr {
    int64_t timestamp;
    uint16_t checksum;
    union {
	int64_t hid;
    } u;
} Msghdr;

#define HDRLEN sizeof(struct _msghdr)

class __SpioInternComsumer : public SpioComsumer {
 public:
    __SpioInternComsumer();
    ~__SpioInternComsumer();

    int Fd();
    KMQTEST int SetOption(int op, ...);
    int GetOption(int op, ...);
    int FlushCache();
    int CacheSize();
    KMQTEST int Connect(const string &appname, const string &remoteaddr);
    KMQTEST int Close();

    KMQTEST int RecvMsg(string &msg, string &rt);
    KMQTEST int SendMsg(const string &msg, const string &rt);
    KMQTEST int SendMsg(const char *data, uint32_t len, const string &rt);
    
    KMQTEST int RecvMsg(Msghdr *hdr, string &msg, string &rt);
    KMQTEST int SendMsg(const Msghdr *hdr, const string &msg, const string &rt);

    KMQTEST int RecvMsg(struct appmsg *raw);
    KMQTEST int SendMsg(const struct appmsg *raw);
    
 private:
    int _to_msec;
    int options;
    string _roleid;
    string _appname, _apphost;
    Conn *internconn;
    proto_parser pp;
};



class __SpioInternProducer : public SpioProducer {
 public:
    __SpioInternProducer();
    ~__SpioInternProducer();

    int Fd();
    KMQTEST int SetOption(int op, ...);
    int GetOption(int op, ...);
    int FlushCache();
    int CacheSize();
    KMQTEST int Connect(const string &appname, const string &remoteaddr);
    KMQTEST int Close();

    KMQTEST int SendMsg(const string &msg);
    KMQTEST int RecvMsg(string &msg);
    KMQTEST int SendMsg(const char *data, uint32_t len);
    
    KMQTEST int SendMsg(const Msghdr *hdr, const string &msg);
    KMQTEST int RecvMsg(Msghdr *hdr, string &msg);

    KMQTEST int SendMsg(const struct appmsg *raw);
    KMQTEST int RecvMsg(struct appmsg *raw);


 private:
    int _to_msec;
    int options;
    string _roleid;
    string _appname, _apphost;
    Conn *internconn;
    proto_parser pp;
};


}
 

#endif
