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


// test sync client api
static int app_sync_client(void *arg_) {
    __SpioInternProducer client;
    int i;

    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }
    if (client.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    for (i = 0; i < cnt; i++) {
	string req;
	EXPECT_TRUE(client.SendMsg(req) == 0);
    }
    for (i = 0; i < cnt; i++) {
	string resp;
	EXPECT_TRUE(client.RecvMsg(resp) == 0);
	EXPECT_TRUE(resp.size() == 0);
    }
    client.Close();
    return 0;
}
class MyAppServerEx1 : public McHandler {
public:
    MyAppServerEx1() {
	mycnt = 0;
	dropped = 0;
    }
    ~MyAppServerEx1() {}
    int DropMsg(const char *data, int len, int ev);
    int ServeKMQ(ReqReader &rr, ResponseWriter &rw);
private:
    int mycnt;
    int dropped;
};

int MyAppServerEx1::DropMsg(const char *data, int len, int ev) {
    dropped++;
    return 0;
}

int MyAppServerEx1::ServeKMQ(ReqReader &rr, ResponseWriter &rw) {
    rw.Send(rr.Data(), rr.Len(), rr.Route());
    mycnt++;
    return 0;
}


class MyAppClientEx1 : public MpHandler {
public:
    MyAppClientEx1() {
	mycnt = 0;
	pmutex_init(&lock);
    }
    ~MyAppClientEx1() {
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

int MyAppClientEx1::ServeKMQ(const char *data, int len) {
    MyHdr *hdr = (MyHdr *)data;
    const char *__data = data + MYHDRLEN;
    int __len = len - MYHDRLEN;
    EXPECT_EQ(hdr->checksum, crc16(__data, __len));
    pmutex_lock(&lock);
    mycnt++;
    pmutex_unlock(&lock);
    return 0;
}


static int app_sync_server(void *arg_) {
    __SpioInternComsumer server;
    int i;
    
    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }
    if (server.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    for (i = 0; i < cnt; i++) {
	string msg, rt;
	EXPECT_TRUE(server.RecvMsg(msg, rt) == 0);
	EXPECT_TRUE(server.SendMsg(msg, rt) == 0);
    }
    server.Close();
    return 0;
}

static int app_async_client(void *arg_) {
    MyAppClientEx1 appclient;
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


static int test_async_sync() {
    Rgm *rgm = NewRegisterManager(NULL);
    MyAppServerEx1 appserver;
    MuxComsumer *ms = NULL;
    MuxProducer *mc = NULL;
	
    struct mux_conf arg = {
	1,
	200,
	100,
    };
    OSThread t[4];

    randstr(buffer, buflen);
    ag = new AppCtx();
    ms = NewMuxComsumer();
    mc = NewMuxProducer();
    ms->SetHandler(&appserver);
    app_conf.appid = appname;
    ag->InitFromConf(&app_conf);
    ag->EnableRegistry(rgm);
    t[0].Start(rgm_thread, rgm);
    t[1].Start(appcontext_thread, NULL);

    // async server -- sync client
    ms->Setup(appname, apphost, &arg);
    ms->StartServe();
    usleep(100000);
    t[2].Start(app_sync_client, NULL);
    t[2].Stop();
    ms->Stop();
    delete ms;
    
    //sync server -- async client	
    t[2].Start(app_sync_server, NULL);
    mc->Setup(appname, apphost);
    mc->StartServe();
    usleep(100000);
    t[3].Start(app_async_client, mc);
    t[3].Stop();
    t[2].Stop();
    mc->Stop();
    delete mc;
    
    ag->Stop();
    rgm->Stop();
    t[0].Stop();
    t[1].Stop();
    delete ag;
    delete rgm;
	
    return 0;
}


TEST(api, mix) {
     test_async_sync();
}
