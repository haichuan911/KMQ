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
#include "api.h"


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


SpioComsumer *NewSpioComsumer() {
    __SpioInternComsumer *svr = new (std::nothrow) __SpioInternComsumer();
    return svr;
}

__SpioInternComsumer::__SpioInternComsumer() :
    _to_msec(0), options(0), internconn(NULL)
{
    route_genid(_roleid);
}

__SpioInternComsumer::~__SpioInternComsumer() {
    if (internconn)
	delete internconn;
}

int __SpioInternComsumer::Fd() {
    if (internconn)
	return internconn->Fd();
    return -1;
}

int __SpioInternComsumer::CacheSize() {
    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    return internconn->CacheSize(SO_READCACHE);
}

int __SpioInternComsumer::FlushCache() {
    int ret = 0;

    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    while ((ret = internconn->Flush()) < 0 && errno == EAGAIN) {}
    return ret;
}


int __SpioInternComsumer::Connect(const string &appname, const string &apphost) {
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
    ret = __registe_role(internconn, appname, _roleid, ROLE_APPDISPATCHER);
    if (ret < 0) {
	Close();
	errno = KMQ_EREGISTRY;
	return -1;
    }
    _appname = appname;
    _apphost = apphost;
    return 0;
}

int __SpioInternComsumer::Close() {
    pp._reset_recv();
    pp._reset_send();
    if (internconn) {
	internconn->Close();
	delete internconn;
    }
    internconn = NULL;
    return 0;
}


int __SpioInternComsumer::SetOption(int opt, ...) {
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


int __SpioInternComsumer::GetOption(int opt, ...) {

    return 0;
}

int __SpioInternComsumer::RecvMsg(string &msg, string &rt) {
    int ret = 0;
    struct appmsg raw = {};

    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if ((ret = RecvMsg(&raw)) == 0) {
	msg.clear();
	rt.clear();
	if (raw.s.len) {
	    msg.assign(raw.s.data, raw.s.len);
	    mem_free(raw.s.data, raw.s.len);
	}
	rt.assign((char *)&raw.hdr, sizeof(raw.hdr));
	rt.append((char *)raw.rt, appmsg_rtlen(raw.rt->ttl));
	mem_free(raw.rt, appmsg_rtlen(raw.rt->ttl));
    }
    return ret;
}


int __SpioInternComsumer::SendMsg(const string &msg, const string &rt) {
    return SendMsg(msg.data(), msg.size(), rt);
}


int __SpioInternComsumer::RecvMsg(Msghdr *hdr, string &msg, string &rt) {
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
	msg.clear();
	rt.clear();
	if (raw.s.len < HDRLEN) {
	    if (raw.s.len)
		mem_free(raw.s.data, raw.s.len);
	    mem_free(raw.rt, appmsg_rtlen(raw.rt->ttl));
	    errno = KMQ_EPACKAGE;
	    return -1;
	}
	memcpy((char *)hdr, raw.s.data, HDRLEN);
	if (raw.s.len > HDRLEN)
	    msg.assign(raw.s.data + HDRLEN, raw.s.len - HDRLEN);
	rt.assign((char *)&raw.hdr, sizeof(raw.hdr));
	rt.append((char *)raw.rt, appmsg_rtlen(raw.rt->ttl));
	mem_free(raw.s.data, raw.s.len);
	mem_free(raw.rt, appmsg_rtlen(raw.rt->ttl));
    }
    return ret;
}


int __SpioInternComsumer::SendMsg(const Msghdr *hdr, const string &msg, const string &rt) {
    string tmpmsg;
    
    if (!hdr) {
	errno = EINVAL;
	return -1;
    }
    tmpmsg.assign((char *)hdr, HDRLEN);
    tmpmsg += msg;
    return SendMsg(tmpmsg.data(), tmpmsg.size(), rt);
}



int __SpioInternComsumer::SendMsg(const char *data, uint32_t len, const string &rt) {
    int rtlen = 0;
    struct appmsg raw = {};

    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    
    if (!data) {
	errno = EINVAL;
	return -1;
    }
    rtlen = rt.size() - sizeof(*raw.rt) - sizeof(raw.hdr);
    if (rtlen < 2 * RTLEN || (rtlen % RTLEN != 0)) {
	errno = EINVAL;
	return -1;
    }
    raw.s.len = len;
    raw.s.data = const_cast<char *>(data);
    raw.hdr = *((struct kmqhdr *)rt.data());
    raw.rt = (struct appmsg_rt *)(rt.data() + sizeof(raw.hdr));
    return SendMsg(&raw);
}

int __SpioInternComsumer::RecvMsg(struct appmsg *raw) {
    int ret = 0;
    struct appmsg_rt *rt = NULL;

    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!raw) {
	errno = EINVAL;
	return -1;
    }

    if ((ret = pp._recv_massage(internconn, &raw->hdr, &raw->s)) == 0) {
	if (!(rt = (struct appmsg_rt *)mem_zalloc(appmsg_rtlen(raw->hdr.ttl)))) {
	    mem_free(raw->s.data, raw->s.len);
	    errno = EAGAIN;
	    return -1;
	}
	raw->s.len -= RTLEN * raw->hdr.ttl;
	rt->ttl = raw->hdr.ttl;
	memcpy(rt->__padding, raw->s.data + raw->s.len, RTLEN * raw->hdr.ttl);
	raw->rt = rt;
	if (!raw->s.len) {
	    mem_free(raw->s.data, RTLEN * raw->hdr.ttl);
	    raw->s.data = NULL;
	}
    } else if (ret < 0 && errno != EAGAIN) {
	pp._reset_recv();
    }

    internconn->SetSockOpt(SO_QUICKACK);
    return ret;
}


int __SpioInternComsumer::SendMsg(const struct appmsg *raw) {
    int ret = 0;
    struct slice s[2];
    struct kmqhdr hdr = {};
    
    if (!internconn) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!raw || !raw->rt) {
	errno = EINVAL;
	return -1;
    }

    hdr = raw->hdr;
    hdr.ttl = raw->rt->ttl;
    hdr.size = hdr.ttl * RTLEN + raw->s.len;
    s[0] = raw->s;
    s[1].len = hdr.ttl * RTLEN;
    s[1].data = raw->rt->__padding;
    if ((ret = pp._send_massage(internconn, &hdr, 2, (struct slice *)&s)) < 0 && errno != EAGAIN)
	pp._reset_send();

    return ret;    
}


}
