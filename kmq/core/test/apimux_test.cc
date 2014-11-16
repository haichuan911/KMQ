#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <gtest/gtest.h>
#include "log.h"
#include "api.h"
#include "CRC.h"
#include "memalloc.h"
#include "appctx.h"
#include "osthread.h"


using namespace kmq;
static AppCfg app_conf;


static int randstr(char *buf, int len) {
    int i, idx;
    char token[] = "qwertyuioplkjhgfdsazxcvbnm1234567890";
    for (i = 0; i < len; i++) {
	idx = rand() % strlen(token);
	buf[i] = token[idx];
    }
    return 0;
}

struct MyHdr {
    int64_t timestamp;
    uint16_t checksum;
};
#define MYHDRLEN sizeof(struct MyHdr)



#define buflen 1024
static char buffer[buflen] = {};
static string appname = "testapp";
static string apphost = "127.0.0.1:20009";
static AppCtx *ag = NULL;
static volatile int cnt = 20;
static volatile int agstarted = 0, rmstarted = 0;


static int rgm_thread(void *arg_) {
    Rgm *rgm = (Rgm *)arg_;
    if (rgm->Listen(apphost) < 0) {
	return -1;
    }
    rmstarted = 1;
    rgm->Start();
    KMQLOG_INFO("rgm stop");
    rmstarted = 0;
    return 0;
}

static int appcontext_thread(void *arg_) {
    while (!rmstarted) {
	/* waitting ... */
    }
    agstarted = 1;
    ag->Start();
    KMQLOG_INFO("ag stop");
    agstarted = 0;
    return 0;
}


static volatile int _stopping;

static int app_server1(void *arg_) {
    __SpioInternComsumer server;
    struct appmsg req = {};

    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }
    while (!_stopping) {
	switch (rand() % 4) {
	case 0:
	    if (server.RecvMsg(&req) == 0) {
		server.SendMsg(&req);
		if (req.s.len)
		    free(req.s.data);
		free(req.rt);
	    }
	    break;
	case 1:
	    server.Close();
	    usleep(10000);
	    break;
	case 2:
	    server.Connect(appname, apphost);
	    server.SetOption(OPT_NONBLOCK, 1);
	    break;
	}
    }
    return 0;
}



static int app_client1(void *arg_) {
    __SpioInternProducer client;
    struct appmsg req = {};
    struct appmsg resp = {};
    int sendcnt = 0, recvcnt = 0;

    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }
    while (sendcnt < cnt) {
	switch (rand() % 4) {
	case 0:
	    if (sendcnt < cnt) {
		req.s.len = (rand() % buflen) + HDRLEN;
		req.s.data = buffer;
		if (client.SendMsg(&req) == 0)
		    sendcnt++;
	    }
	    break;
	case 1:
	    if (client.RecvMsg(&resp) == 0) {
		if (resp.s.len)
		    free(resp.s.data);
		recvcnt++;
	    }
	    break;
	case 2:
	    client.Close();
	    usleep(10000);
	    break;
	case 3:
	    client.Connect(appname, apphost);
	    client.SetOption(OPT_NONBLOCK, 1);
	    break;
	}
    }
    EXPECT_TRUE(sendcnt > 0);
    _stopping = 1;
    return 0;
}


class MyAppServerEx : public McHandler {
public:
    MyAppServerEx() {
	mycnt = 0;
	dropped = 0;
    }
    ~MyAppServerEx() {}
    int DropMsg(const char *data, int len, int ev);
    int ServeKMQ(ReqReader &rr, ResponseWriter &rw);
private:
    int mycnt;
    int dropped;
};

int MyAppServerEx::DropMsg(const char *data, int len, int ev) {
    dropped++;
    printf("fucking drop %d\n", ev);
    return 0;
}

int MyAppServerEx::ServeKMQ(ReqReader &rr, ResponseWriter &rw) {
    rw.Send(rr.Data(), rr.Len(), rr.Route());
    rw.Send(rr.Data(), rr.Len(), rr.Route());
    mycnt++;
    mycnt++;
    return 0;
}


class MyAppClientEx : public MpHandler {
public:
    MyAppClientEx() {
	mycnt = 0;
	pmutex_init(&lock);
    }
    ~MyAppClientEx() {
	pmutex_destroy(&lock);
    }
    int recvcnt() {
	return mycnt;
    }
    int TimeOut(int to_msec) {

	return 0;
    }
    int ServeKMQ(const char *data, int len);

private:
    int mycnt;
    pmutex_t lock;
};

int MyAppClientEx::ServeKMQ(const char *data, int len) {
    MyHdr *hdr = (MyHdr *)data;
    const char *__data = data + MYHDRLEN;
    int __len = len - MYHDRLEN;
    EXPECT_EQ(hdr->checksum, crc16(__data, __len));
    pmutex_lock(&lock);
    mycnt++;
    pmutex_unlock(&lock);
    return 0;
}

// test mux client api
static int app_client3(void *arg_) {
    MyAppClientEx appclient;
    MuxProducer *mc = (MuxProducer *)arg_;
    string msg;
    MyHdr hdr = {};
    int len = 0, i;

    for (i = 0; i < cnt; i++) {
	msg.clear();
	len = (rand() % (buflen - MYHDRLEN)) + MYHDRLEN;
	hdr.checksum = crc16(buffer, len - MYHDRLEN);
	msg.append((char *)&hdr, MYHDRLEN);
	msg.append(buffer, len - MYHDRLEN);
	mc->SendRequest(msg.data(), msg.size(), &appclient);
    }
    while (appclient.recvcnt() != cnt)
	usleep(1);
    return 0;
}

// test mux client api after kmq server down
static int app_client4(void *arg_) {
    MyAppClientEx appclient;
    MuxProducer *mc = (MuxProducer *)arg_;
    string msg;
    MyHdr hdr = {};
    int len = 0, i;

    for (i = 0; i < cnt; i++) {
	msg.clear();
	len = (rand() % (buflen - MYHDRLEN)) + MYHDRLEN;
	hdr.checksum = crc16(buffer, len - MYHDRLEN);
	msg.append((char *)&hdr, MYHDRLEN);
	msg.append(buffer, len - MYHDRLEN);
	mc->SendRequest(msg.data(), msg.size(), &appclient);
    }
    usleep(200000);
    return 0;
}


static int test_apimux() {
    Rgm *rgm = NewRegisterManager(NULL);
    MyAppServerEx appserver;
    MuxComsumer *ms = NULL;
    MuxProducer *mc = NULL;
    OSThread t[4];

    randstr(buffer, buflen);
    ag = new AppCtx();
    ms = NewMuxComsumer();
    mc = NewMuxProducer();
    ms->SetHandler(&appserver);
    app_conf.appid = appname;
    app_conf.msg_timeout_msec = 200;
    ag->InitFromConf(&app_conf);
    ag->EnableRegistry(rgm);
    t[0].Start(rgm_thread, rgm);
    t[1].Start(appcontext_thread, NULL);

    t[2].Start(app_server1, NULL);
    t[3].Start(app_client1, NULL);
    t[2].Stop();
    t[3].Stop();

    ms->Setup(appname, apphost);
    ms->StartServe();
    usleep(100000);
    ms->Stop();

    // testing mux server and mux client
    ms->Setup(appname, apphost);
    EXPECT_TRUE(ms->Setup(appname, apphost) == -1 && errno == KMQ_EINTERN);
    ms->StartServe();
    mc->Setup(appname, apphost);
    mc->StartServe();
    usleep(100000);

    t[2].Start(app_client3, mc);
    t[3].Start(app_client3, mc);
    t[2].Stop();
    t[3].Stop();

    // testing memleak when ~MuxComsumer()
    usleep(100000);
    t[2].Start(app_client4, mc);
    t[3].Start(app_client4, mc);
    usleep(10000);
    ag->Stop();
    rgm->Stop();
    t[0].Stop();
    t[1].Stop();
    ms->Stop();
    mc->Stop();
    t[2].Stop();
    t[3].Stop();
    delete ms;
    delete mc;

    delete ag;
    delete rgm;
    return 0;
}


TEST(api, mux) {
    test_apimux();
}
