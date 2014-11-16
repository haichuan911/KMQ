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

static int sleeptime = 20;

struct MyHdr {
    int64_t timestamp;
    uint16_t checksum;
};
#define MYHDRLEN sizeof(struct MyHdr)

class TrendAppServer : public McHandler {
public:
    TrendAppServer() : drop(0), recv_packages(0)
    {
    }
    ~TrendAppServer() {}
    int DropMsg(const char *data, int len, int ev);
    int ServeKMQ(ReqReader &rr, ResponseWriter &rw);
private:
    int drop;
    int recv_packages;
};


int TrendAppServer::DropMsg(const char *data, int len, int ev) {
    drop++;
    return 0;
}

int TrendAppServer::ServeKMQ(ReqReader &rr, ResponseWriter &rw) {
    int i = 0;
    int64_t ett = 0;
    ett = rt_mstime() + sleeptime;
    while (rt_mstime() < ett)
	i++;
    rw.Send(rr.Data(), rr.Len(), rr.Route());
    recv_packages++;
    return 0;
}


class TrendAppClient : public MpHandler {
public:
    TrendAppClient() : rtt(0), recv_packages(0), error_packages(0) {
	pmutex_init(&lock);
    }
    ~TrendAppClient() {
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

int TrendAppClient::TimeOut(int to_msec) {
    return 0;
}

int TrendAppClient::ServeKMQ(const char *data, int len) {
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

typedef struct trend_env {
    MuxProducer *mux_cli;
    TrendAppClient *appclient;
} trend_env_t;


int kmq_trend_server(void *arg_) {
    TrendAppServer appserver;
    struct mux_conf arg = {
	g_servers,
	g_servers * 2,
	100,
    };
    MuxComsumer *mux_svr = NewMuxComsumer();

    if (!mux_svr)
	return -1;
    mux_svr->SetHandler(&appserver);
    mux_svr->Setup(appname, kmqsvrhost, &arg);
    mux_svr->StartServe();
    while (!_stopping)
	sleep(1);
    mux_svr->Stop();
    delete mux_svr;
    return 0;
}


static int trend_send_worker(void *arg_) {
    trend_env_t *env = (trend_env_t *)arg_;
    MuxProducer *mux_cli = env->mux_cli;
    TrendAppClient *appclient = env->appclient;
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
		hdr.checksum = crc16(buffer + MYHDRLEN, len - MYHDRLEN);
	    sendcnt++;
	    memcpy(buffer, &hdr, MYHDRLEN);
	    mux_cli->SendRequest(buffer, len, appclient);
	}
	rt_usleep(intern_tt);
    }
    return sendcnt;
}


int kmq_trend_client(void *arg_) {
    trend_env_t env;
    struct mux_conf cli_conf = {
	g_clients,
	3 * g_clients,
	0,
    };
    MuxProducer *mux_cli = NULL;
    TrendAppClient *appclient = NULL;
    int i, sendcnt = 0, recvcnt = 0, lost;
    OSThread *thread = NULL;
    vector<OSThread *> send_workers;
    vector<OSThread *>::iterator it;
    
    mux_cli = NewMuxProducer();
    appclient = new TrendAppClient();
    if (!mux_cli || !appclient)
	return -1;
    mux_cli->Setup(appname, kmqclihost, &cli_conf);
    mux_cli->StartServe();
    env.mux_cli = mux_cli;
    env.appclient = appclient;

    for (i = 0; i < g_clients; i++) {
	thread = new OSThread();
	send_workers.push_back(thread);
	thread->Start(trend_send_worker, &env);
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
