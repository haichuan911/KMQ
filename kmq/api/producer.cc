#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <uuid/uuid.h>
#include <string.h>
#include <stdarg.h>
#include <kmq/errno.h>
#include "os.h"
#include "net.h"
#include "rid.h"
#include "memalloc.h"
#include "mux.h"


KMQ_DECLARATION_START

static int version = 0x8;

static int __registe_role(Conn *internconn,
			  const string &appname, const string &roleid, int rtype) {
    struct kmqreg rgh = {};
    int nbytes = 0, ret = 0;

    rgh.version = version;
    rgh.rtype = rtype;
    rgh.timeout = 0;
    rgh_unparse(roleid, &rgh);
    strncpy(rgh.appname, appname.data(), MAX_APPNAME_LEN);
    
    // Second. send registe request.
    while (nbytes < REGHDRLEN) {
	ret = internconn->Write((char *)&rgh + nbytes, REGHDRLEN - nbytes);
	if (ret < 0)
	    return -1;
	nbytes += ret;
    }
    
    return 0;
}

SpioProducer *NewSpioProducer() {
    __SpioInternProducer *cli = new (std::nothrow) __SpioInternProducer();
    return cli;
}

__SpioInternProducer::__SpioInternProducer() :
    _to_msec(0), options(0), internconn(NULL)
{
    route_genid(_roleid);
}

__SpioInternProducer::~__SpioInternProducer() {
    if (internconn)
	delete internconn;
}

int __SpioInternProducer::Fd() {
    if (internconn)
	return internconn->Fd();
    return -1;
}

int __SpioInternProducer::CacheSize() {
    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    return internconn->CacheSize(SO_READCACHE);
}


int __SpioInternProducer::FlushCache() {
    int ret = 0;

    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    while ((ret = internconn->Flush()) < 0 && errno == EAGAIN) {}
    return ret;
}


int __SpioInternProducer::Connect(const string &appname, const string &apphost) {
    int ret = 0;

    if (internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!(internconn = DialTCP("tcp", "", apphost))) {
	errno = KMQ_ENETUNREACH;
	return -1;
    }
    internconn->SetSockOpt(SO_NODELAY, 1);
    ret = __registe_role(internconn, appname, _roleid, ROLE_APPRECEIVER);
    if (ret < 0) {
	Close();
	errno = KMQ_EREGISTRY;
	return -1;
    }
    _appname = appname;
    _apphost = apphost;
    return 0;
}


int __SpioInternProducer::Close() {
    pp._reset_recv();
    pp._reset_send();
    if (internconn) {
	internconn->Close();
	delete internconn;
    }
    internconn = NULL;
    return 0;
}



int __SpioInternProducer::SetOption(int opt, ...) {
    va_list ap;
    
    switch (opt) {
    case OPT_TIMEOUT:
	{
	    int to_msec = 0;
	    if (!internconn) {
		errno = KMQ_EINTERN;
		return -1;
	    }
	    va_start(ap, opt);
	    to_msec = va_arg(ap, int);
	    va_end(ap);
	    if (to_msec < 0) {
		errno = EINVAL;
		return -1;
	    }
	    _to_msec = to_msec;
	    return internconn->SetSockOpt(SO_TIMEOUT, _to_msec);
	}
    case OPT_AUTORECONNECT:
	{
	    // deprecated option
	    options |= OPT_AUTORECONNECT;
	    break;
	}
    case OPT_NONBLOCK:
	{
	    int flags = 0;
	    if (!internconn) {
		errno = KMQ_EINTERN;
		return -1;
	    }
	    va_start(ap, opt);
	    flags = va_arg(ap, int);
	    va_end(ap);
	    if (flags)
		internconn->SetSockOpt(SO_NONBLOCK, 1);
	    else
		internconn->SetSockOpt(SO_NONBLOCK, 0);
	    break;
	}
    case OPT_SOCKCACHE:
	{
	    if (!internconn) {
		errno = KMQ_EINTERN;
		return -1;
	    }
	    internconn->SetSockOpt(SO_READCACHE, 0);
	    internconn->SetSockOpt(SO_WRITECACHE, 0);
	    break;
	}
    }
    return 0;
}


int __SpioInternProducer::GetOption(int opt, ...) {

    return 0;
}

int __SpioInternProducer::SendMsg(const string &msg) {
    return SendMsg(msg.data(), msg.size());
}

int __SpioInternProducer::RecvMsg(string &msg) {
    int ret = 0;
    struct appmsg raw = {};
    
    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if ((ret = RecvMsg(&raw)) == 0) {
	if (raw.s.len) {
	    msg.assign(raw.s.data, raw.s.len);
	    mem_free(raw.s.data, raw.s.len);
	}
    }
    return ret;
}


int __SpioInternProducer::SendMsg(const Msghdr *hdr, const string &msg) {
    string tmpmsg;

    if (!hdr) {
	errno = EINVAL;
	return -1;
    }
    
    tmpmsg.assign((char *)hdr, HDRLEN);
    tmpmsg += msg;
    return SendMsg(tmpmsg.data(), tmpmsg.size());
}


int __SpioInternProducer::RecvMsg(Msghdr *hdr, string &msg) {
    int ret = 0;
    struct appmsg raw = {};
    
    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!hdr) {
	errno = EINVAL;
	return -1;
    }
    
    if ((ret = RecvMsg(&raw)) == 0) {
	if (raw.s.len < HDRLEN) {
	    if (raw.s.len)
		mem_free(raw.s.data, raw.s.len);
	    errno = KMQ_EPACKAGE;
	    return -1;
	}
	memcpy((char *)hdr, raw.s.data, HDRLEN);
	if (raw.s.len > HDRLEN)
	    msg.assign(raw.s.data + HDRLEN, raw.s.len - HDRLEN);
	mem_free(raw.s.data, raw.s.len);
    }
    return ret;
}



int __SpioInternProducer::SendMsg(const char *data, uint32_t len) {
    struct appmsg raw = {};

    if (!data) {
	errno = EINVAL;
	return -1;
    }
    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    raw.s.data = const_cast<char *>(data);
    raw.s.len = len;
    return SendMsg(&raw);
}

int __SpioInternProducer::SendMsg(const struct appmsg *raw) {
    int ret = 0;
    struct slice s[2];
    struct kmqrt rt = {};
    struct kmqhdr hdr = {};

    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!raw) {
	errno = EINVAL;
	return -1;
    }

    route_unparse(_roleid.data(), &rt);
    rt.u.env.sendstamp = (uint32_t)rt_mstime();
    hdr = raw->hdr;
    hdr.timestamp = rt_mstime();
    hdr.ttl = 1;
    hdr.size = RTLEN + raw->s.len;
    s[0] = raw->s;
    s[1].len = RTLEN;
    s[1].data = (char *)&rt;
    if ((ret = pp._send_massage(internconn, &hdr, 2, (struct slice *)&s)) < 0 && errno != EAGAIN)
	pp._reset_send();

    return ret;
}


int __SpioInternProducer::RecvMsg(struct appmsg *raw) {
    int ret = 0;

    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!raw) {
	errno = EINVAL;
	return -1;
    }

    if ((ret = pp._recv_massage(internconn, &raw->hdr, &raw->s)) == 0) {
	raw->s.len -= RTLEN * raw->hdr.ttl;
	raw->rt = NULL;
	if (!raw->s.len) {
	    mem_free(raw->s.data, raw->hdr.size);
	    raw->s.data = NULL;
	}
    } else if (ret < 0 && errno != EAGAIN) {
	pp._reset_recv();
    }
    internconn->SetSockOpt(SO_QUICKACK);
    return ret;
}



}
