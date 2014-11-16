#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/compatapi.h>
#include "os.h"
#include "api.h"

KMQ_DECLARATION_START


CSpioApi::CSpioApi() :
    m_joined(false), m_msgid(0), m_to_msec(0),
    internserver(NULL), internclient(NULL)
{

}

CSpioApi::~CSpioApi() {
    if (internserver)
	delete internserver;
    if (internclient)
	delete internclient;
}

int CSpioApi::init(const string &grouphost) {
    if (m_joined) {
	errno = KMQ_EINTERN;
	return -1;
    }
    m_grouphost = grouphost;
    return 0;
}

#if defined(__KMQ_UT__)
int CSpioApi::fd() {
    if (!m_joined) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (internserver)
	return internserver->Fd();
    else if (internclient)
	return internclient->Fd();

    return 0;
}
#endif

int CSpioApi::join_client(const string &groupname) {
    if (m_joined || internserver) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (internclient) {
	if (m_groupname == groupname)
	    return 0;
    } else {
	if (!(internclient = NewSpioProducer())) {
	    errno = ENOMEM;
	    return -1;
	}
    }
    m_groupname = groupname;
    return rejoin();
}

int CSpioApi::join_server(const string &groupname) {
    if (m_joined || internclient) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (internserver) {
	if (m_groupname == groupname)
	    return 0;
    } else {
	if (!(internserver = NewSpioComsumer())) {
	    errno = ENOMEM;
	    return -1;
	}
    }
    m_groupname = groupname;
    return rejoin();
}

int CSpioApi::rejoin() {

    if (internserver) {
	internserver->Close();
	while (internserver->Connect(m_grouphost, m_groupname) != 0)
	    rt_usleep(100000);
	if (m_to_msec)
	    internserver->SetOption(OPT_TIMEOUT, m_to_msec);
	m_joined = true;
    } else if (internclient) {
	internclient->Close();
	while (internclient->Connect(m_grouphost, m_groupname) != 0)
	    rt_usleep(100000);
	if (m_to_msec)
	    internclient->SetOption(OPT_TIMEOUT, m_to_msec);
	m_joined = true;
    }
    return 0;
}


int CSpioApi::recv(string &msg, int timeout) {
    int ret = 0;
    Msghdr hdr = {};
    __SpioInternProducer *client = NULL;
    __SpioInternComsumer *server = NULL;
    uint64_t end_tt = timeout ? (uint64_t)rt_mstime() + timeout : ~(0);

    if (!m_joined) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (internserver) {
	server = (__SpioInternComsumer *)internserver;
	if (!m_to_msec || m_to_msec != timeout) {
	    m_to_msec = timeout;
	    server->SetOption(OPT_TIMEOUT, timeout);
	}
	do {
	    ret = server->RecvMsg(&hdr, msg, cur_rt);
	} while (ret < 0 && errno == EAGAIN && (uint64_t)rt_mstime() < end_tt);
	if (ret == 0) {
	    m_msgid = hdr.timestamp;
	}
    } else if (internclient) {
	client = (__SpioInternProducer *)internclient;
	if (!m_to_msec || m_to_msec != timeout) {
	    m_to_msec = timeout;
	    client->SetOption(OPT_TIMEOUT, timeout);
	}
	do {
	    ret = client->RecvMsg(&hdr, msg);
	    if (ret == 0 && hdr.timestamp != m_msgid) {
		msg.clear();
		ret = -1;
		errno = EAGAIN;
		end_tt = timeout ? (uint64_t)rt_mstime() + timeout : ~(0);
	    }
	} while (ret < 0 && (errno == EAGAIN && (uint64_t)rt_mstime() < end_tt));
    }
    if (ret < 0 && errno == EPIPE) {
	rejoin();
	errno = KMQ_EKMQDOWN;
    }
    return ret;
}


int CSpioApi::send(const string &msg, int timeout) {
    int ret = 0;
    Msghdr hdr = {};
    __SpioInternProducer *client = NULL;
    __SpioInternComsumer *server = NULL;
    uint64_t end_tt = timeout ? ((uint64_t)rt_mstime() + timeout) : ~(0);

    if (!m_joined) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (internserver && cur_rt.empty()) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (internserver) {
	server = (__SpioInternComsumer *)internserver;
	hdr.timestamp = m_msgid;
	if (!m_to_msec || m_to_msec != timeout) {
	    m_to_msec = timeout;
	    server->SetOption(OPT_TIMEOUT, timeout);
	}
	do {
	    ret = server->SendMsg(&hdr, msg, cur_rt);
	} while (ret < 0 && errno == EAGAIN && (uint64_t)rt_mstime() < end_tt);
	cur_rt.clear();
    } else if (internclient) {
	m_msgid++;
	hdr.timestamp = m_msgid;
	client = (__SpioInternProducer *)internclient;
	if (!m_to_msec || m_to_msec != timeout) {
	    m_to_msec = timeout;
	    client->SetOption(OPT_TIMEOUT, timeout);
	}
	do {
	    ret = client->SendMsg(&hdr, msg);
	} while (ret < 0 && errno == EAGAIN && (uint64_t)rt_mstime() < end_tt);
    }
    if (ret < 0 && errno == EPIPE) {
	rejoin();
	errno = KMQ_EKMQDOWN;
    }
    return ret;
}




void CSpioApi::terminate() {
    if (internserver) {
	internserver->Close();
	delete internserver;
	internserver = NULL;
    } else if (internclient) {
	internclient->Close();
	delete internclient;
	internclient = NULL;
    }
    m_joined = false;
}


}
