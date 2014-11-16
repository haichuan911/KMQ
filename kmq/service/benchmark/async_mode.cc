#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kmq/errno.h>
#include <kmq/kmqmux.h>
#include "os.h"
#include "log.h"
#include "CRC.h"
#include "pmutex.h"
#include "osthread.h"
#include "epoller.h"
#include "api.h"
#include "benchmark.h"


using namespace kmq;

struct MyHdr {
    int64_t timestamp;
    uint16_t checksum;
};
#define MYHDRLEN sizeof(struct MyHdr)

class MyAppServer : public McHandler {
public:
    MyAppServer() : drop(0), recv_packages(0)
    {
    }
    ~MyAppServer() {}
    int DropMsg(const char *msg, int len, int ev);
    int ServeKMQ(ReqReader &rr, ResponseWriter &rw);
private:
    int drop;
    int recv_packages;
};

int MyAppServer::DropMsg(const char *msg, int len, int ev) {
    drop++;
    return 0;
}

int MyAppServer::ServeKMQ(ReqReader &rr, ResponseWriter &rw) {
    rw.Send(rr.Data(), rr.Len(), rr.Route());
    recv_packages++;
    if (recv_packages % 1000 == 0)
	printf("mxserver recv %d packages\n", recv_packages);
    return 0;
}


class MyAppClient : public MpHandler {
public:
    MyAppClient() : rtt(0), recv_packages(0), error_packages(0) {
	pmutex_init(&lock);
    }
    ~MyAppClient() {
	pmutex_destroy(&lock);
    }
    int Recvs() {
	return recv_packages;
    }
    int Rtt() {
	return rtt;
    }
    int TimeOut(int to_msec);
    int ServeKMQ(const char *data, int len);

private:
    int rtt;
    int recv_packages, error_packages;
    pmutex_t lock;
};

int MyAppClient::TimeOut(int to_msec) {
    return 0;
}

int MyAppClient::ServeKMQ(const char *data, int len) {
    int error = 0;
    int ett = rt_mstime();
    MyHdr *hdr = (MyHdr *)data;
    const char *__data = data + MYHDRLEN;
    int __len = len - MYHDRLEN;

    if (g_check == "yes" && hdr->checksum != crc16(__data, __len))
	error = 1;
    pmutex_lock(&lock);
    recv_packages++;
    if (error)
	error_packages++;
    rtt += ett - hdr->timestamp;
    pmutex_unlock(&lock);
    return 0;
}

static volatile int _stopping = 0;

typedef struct async_env {
    MuxProducer *mux_cli;
    MyAppClient *appclient;
} async_env_t;


int kmq_async_server(void *arg_) {
    MyAppServer appserver;
    struct mux_conf arg = {
	g_servers,
	1000 * g_servers,
	100,
    };
    MuxComsumer *mux_svr = NewMuxComsumer();

    if (!mux_svr)
	return -1;
    mux_svr->SetHandler(&appserver);
    mux_svr->Setup(appname, kmqsvrhost, &arg);
    mux_svr->StartServe();
    while (!_stopping)
	usleep(1000);
    mux_svr->Stop();
    delete mux_svr;
    return 0;
}


int async_send_worker(void *arg_) {
    async_env_t *env = (async_env_t *)arg_;
    MuxProducer *mux_cli = env->mux_cli;
    MyAppClient *appclient = env->appclient;
    MyHdr hdr = {};
    int64_t start_tt = 0, end_tt = 0, intern_tt = 0;    
    int len = 0, sendcnt = 0, i;

    start_tt = rt_mstime();
    end_tt = start_tt + g_time;
    intern_tt = 1000000 / g_freq;
    while (rt_mstime() <= end_tt) {
	for (i = 0; i < g_pkgs; i++) {
	    len = rand() % (g_size - MYHDRLEN) + MYHDRLEN;
	    hdr.timestamp = rt_mstime();
	    if (g_check == "yes")
		hdr.checksum = crc16(buffer, len);
	    sendcnt++;
	    memcpy(buffer, &hdr, MYHDRLEN);
	    mux_cli->SendRequest(buffer, len, appclient, 10);
	}
	rt_usleep(intern_tt);
    }
    return sendcnt;
}


int kmq_async_client(void *arg_) {
    async_env_t env;
    struct mux_conf cli_conf = {
	g_clients,
	1000 * g_clients,
	0,
    };
    MuxProducer *mux_cli = NULL;
    MyAppClient *appclient = NULL;
    int i, sendcnt = 0, recvcnt = 0, lost;
    OSThread *thread = NULL;
    vector<OSThread *> send_workers;
    vector<OSThread *>::iterator it;
    
    mux_cli = NewMuxProducer();
    appclient = new MyAppClient();
    if (!mux_cli || !appclient)
	return -1;
    mux_cli->Setup(appname, kmqclihost, &cli_conf);
    mux_cli->StartServe();
    env.mux_cli = mux_cli;
    env.appclient = appclient;

    for (i = 0; i < g_clients; i++) {
	thread = new OSThread();
	send_workers.push_back(thread);
	thread->Start(async_send_worker, &env);
    }
    for (it = send_workers.begin(); it != send_workers.end(); ++it) {
	thread = *it;
	sendcnt += thread->Stop();
	delete thread;
    }
    sleep(2);
    mux_cli->Stop();
    _stopping = 1;

    recvcnt = appclient->Recvs();
    lost = sendcnt - recvcnt;
    if (recvcnt)
	printf("client send: %10d recv: %10d rtt: %2d lost: %.6f tp: %d\n",
	       sendcnt, recvcnt, appclient->Rtt() / recvcnt,
	       (float)lost / (float)sendcnt, recvcnt * 1000 / g_time);
    delete mux_cli;
    delete appclient;
    return 0;
}
