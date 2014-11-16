#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include <kmq/errno.h>
#include "memalloc.h"
#include "CRC.h"
#include "log.h"
#include "osthread.h"
#include "appctx.h"
#include "api.h"

using namespace kmq;

static string appname = "testapp";

#define buflen 1024
static char buffer[buflen] = {};
static string apphost = "127.0.0.1:20001";
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

static volatile int agstarted = 0, rmstarted = 0;
static volatile int cnt = 100;


// test api for AppServer
//    int RecvMsg(string &msg, string &rt);
//    int SendMsg(string &msg, string &rt);
// test api for AppClient
//    int SendMsg(string &msg);
//    int RecvMsg(string &msg);

static volatile int s1_status = 0;
static int app_server1(void *arg_) {
    __SpioInternComsumer server;
    int i;
    
    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }
    if (server.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    s1_status = 1;
    for (i = 0; i < cnt; i++) {
	string msg, rt;
	EXPECT_TRUE(server.RecvMsg(msg, rt) == 0);
	EXPECT_TRUE(server.SendMsg(msg, rt) == 0);
    }
    server.Close();
    return 0;
}


static int app_client1(void *arg_) {
    __SpioInternProducer client;
    int i;

    while (!agstarted || !rmstarted || !s1_status) {
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


// test api for AppServer
//    int RecvMsg(Msghdr *, string &msg, string &rt);
//    int SendMsg(const Msghdr *, string &msg, string &rt);
// test api for AppClient
//    int SendMsg(const Msghdr *, string &msg);
//    int RecvMsg(Msghdr *, string &msg);

static volatile int s3_status = 0;

static int app_server3(void *arg_) {
    __SpioInternComsumer server;
    Msghdr hdr = {};
    int i;
    
    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }    
    if (server.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    s3_status = 1;
    for (i = 0; i < cnt; i++) {
	string msg, rt;
	EXPECT_TRUE(server.RecvMsg(&hdr, msg, rt) == 0);
	EXPECT_TRUE(server.SendMsg(&hdr, msg, rt) == 0);
    }
    server.Close();
    return 0;
}


static int app_client3(void *arg_) {
    __SpioInternProducer client;
    Msghdr hdr = {};
    string req, resp;
    int i, msglen;


    while (!agstarted || !rmstarted || !s3_status) {
	/* waiting ... */
    }
    if (client.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }

    for (i = 0; i < cnt; i++) {
	req.clear();
	if ((msglen = i) > buflen)
	    msglen = buflen;
	req.assign(buffer, msglen);
	hdr.checksum = crc16(req.data(), req.size());
	EXPECT_TRUE(client.SendMsg(&hdr, req) == 0);
    }
    for (i = 0; i < cnt; i++) {
	hdr.checksum = 0;
	resp.clear();
	EXPECT_TRUE(client.RecvMsg(&hdr, resp) == 0);
	EXPECT_EQ(hdr.checksum, crc16(resp.data(), resp.size()));
    }
    client.Close();
    return 0;
}


// test api for AppServer
//    int RecvMsg(struct appmsg *);
//    int SendMsg(struct appmsg *);
// test api for AppClient
//    int SendMsg(struct appmsg *);
//    int RecvMsg(struct appmsg *);

static volatile int s5_status = 0;

static int app_server5(void *arg_) {
    __SpioInternComsumer server;
    int i;
    
    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }    
    if (server.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    s5_status = 1;
    for (i = 0; i < cnt; i++) {
	struct appmsg msg = {};
	EXPECT_TRUE(server.RecvMsg(&msg) == 0);
	EXPECT_TRUE(server.SendMsg(&msg) == 0);
	if (msg.s.len)
	    free(msg.s.data);
	free(msg.rt);
    }
    server.Close();
    return 0;
}


static int app_client5(void *arg_) {
    __SpioInternProducer client;
    struct appmsg req = {};
    struct appmsg resp = {};
    int i, ret;

    while (!agstarted || !rmstarted || !s5_status) {
	/* waiting ... */
    }
    if (client.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
	
    for (i = 0; i < cnt; i++) {
	req.s.len = 0;
	req.s.data = NULL;
	if ((ret = client.SendMsg(&req)) < 0)
	    KMQLOG_ERROR("client send request error");
    }
    for (i = 0; i < cnt; i++) {
	EXPECT_TRUE(client.RecvMsg(&resp) == 0);
	EXPECT_TRUE(NULL == resp.s.data);
	EXPECT_TRUE(0 == resp.s.len);
    }
    client.Close();
    return 0;
}

// test api for AppServer
//    int RecvMsg(string &msg, string &rt);
//    int SendMsg(const char *data, uint32_t len, string &rt);
// test api for AppClient
//    int SendMsg(string &msg);
//    int SendMsg(const char *data, uint32_t len);

static volatile int s7_status = 0;

static int app_server7(void *arg_) {
    __SpioInternComsumer server;
    int i;
    
    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }    
    if (server.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    s7_status = 1;
    for (i = 0; i < cnt; i++) {
	string msg, rt;
	EXPECT_TRUE(server.RecvMsg(msg, rt) == 0);
	EXPECT_TRUE(server.SendMsg(msg.data(), msg.size(), rt) == 0);
    }
    server.Close();
    return 0;
}


static int app_client7(void *arg_) {
    __SpioInternProducer client;
    int i;

    while (!agstarted || !rmstarted || !s7_status) {
	/* waiting ... */
    }
    if (client.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
	
    for (i = 0; i < cnt; i++) {
	string req;
	EXPECT_TRUE(client.SendMsg(req.data(), req.size()) == 0);
    }
    for (i = 0; i < cnt; i++) {
	string resp;
	EXPECT_TRUE(client.RecvMsg(resp) == 0);
	EXPECT_TRUE(resp.size() == 0);
    }
    client.Close();
    return 0;
}


// test api for OPT_TIMEOUT

static volatile int s8_status = 0;

static int app_server8(void *arg_) {
    __SpioInternComsumer server;
    int to_msec = 10;
    string req, rt;

    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }
    if (server.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    s8_status = 1;
    EXPECT_TRUE(server.SetOption(OPT_TIMEOUT, to_msec) == 0);
    EXPECT_TRUE((server.RecvMsg(req, rt) == -1 && errno == EAGAIN));
    server.Close();
    return 0;
}


static int app_client8(void *arg_) {
    __SpioInternProducer client;
    int to_msec = 10;
    string resp;

    while (!agstarted || !rmstarted || !s8_status) {
	/* waiting ... */
    }
    if (client.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    EXPECT_TRUE(client.SetOption(OPT_TIMEOUT, to_msec) == 0);
    EXPECT_TRUE((client.RecvMsg(resp) == -1 && errno == EAGAIN));
    client.Close();
    return 0;
}


// test api for socket error
static volatile int s9_stopped = 0;
static volatile int c9_stopped = 0;
static volatile int s9_status = 0;

static int app_server9(void *arg_) {
    __SpioInternComsumer server;
    int ret = 0, to_msec = 1000;
    string req, rt;

    while (!agstarted || !rmstarted) {
	/* waiting ... */
    }
    if (server.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    s9_status = 1;
    EXPECT_TRUE((ret = server.SetOption(OPT_TIMEOUT, to_msec)) == 0);
    if (ret != 0)
	KMQLOG_ERROR("errno %d", errno);
    EXPECT_TRUE((ret = server.RecvMsg(req, rt)) == 0);
    if (ret != 0)
	KMQLOG_ERROR("errno %d", errno);
    EXPECT_TRUE((ret = server.SendMsg(req, rt)) == 0);
    if (ret != 0)
	KMQLOG_ERROR("errno %d", errno);

    randstr(buffer, buflen);
    write(server.Fd(), buffer, buflen);
    s9_stopped = 1;
    while (agstarted)
	usleep(1);

    // fix here. testing bad socket error
    server.Close();
    return 0;
}


static int app_client9(void *arg_) {
    __SpioInternProducer client;
    string req, resp;
    int ret = 0, to_msec = 10000;


    while (!agstarted || !rmstarted || !s9_status) {
	/* waiting ... */
    }
    if (client.Connect(appname, apphost) < 0) {
	KMQLOG_ERROR("connect to testapp");
	return -1;
    }
    EXPECT_TRUE(client.SetOption(OPT_TIMEOUT, to_msec) == 0);
    req.assign(buffer, rand() % buflen);
    EXPECT_TRUE((ret = client.SendMsg(req)) == 0);
    if (ret != 0)
	KMQLOG_ERROR("errno %d", errno);
    EXPECT_TRUE((ret = client.RecvMsg(resp)) == 0);
    if (ret != 0)
	KMQLOG_ERROR("errno %d", errno);

    write(client.Fd(), buffer, buflen);
    c9_stopped = 1;
    while (agstarted)
	usleep(1);

    // fix here. testing bad socket error
    client.Close();
    return 0;
}


static int test_rawapi(void *arg) {
    AppCtx * ag = new AppCtx();
    Rgm *rgm = NewRegisterManager(NULL);
    OSThread t[5];

    randstr(buffer, buflen);
    app_conf.appid = appname;
    ag->InitFromConf(&app_conf);
    ag->EnableRegistry(rgm);
    rgm->Listen(apphost);
    rgm->StartThread();
    rmstarted = 1;
    ag->StartThread();
    agstarted = 1;

    t[2].Start(app_server1, NULL);
    t[3].Start(app_client1, NULL);
    t[2].Stop();
    t[3].Stop();

    t[2].Start(app_server3, NULL);
    t[3].Start(app_client3, NULL);
    t[2].Stop();
    t[3].Stop();

    t[2].Start(app_server5, NULL);
    t[3].Start(app_client5, NULL);
    t[2].Stop();
    t[3].Stop();

    t[2].Start(app_server7, NULL);
    t[3].Start(app_client7, NULL);
    t[2].Stop();
    t[3].Stop();

    t[2].Start(app_server8, NULL);
    t[3].Start(app_client8, NULL);
    t[2].Stop();
    t[3].Stop();


    t[2].Start(app_server9, NULL);
    t[3].Start(app_client9, NULL);

    while (!s9_stopped || !c9_stopped)
	usleep(1);
    rgm->StopThread();
    rmstarted = 0;
    ag->StopThread();

    delete ag;
    delete rgm;
    agstarted = 0;

    t[2].Stop();
    t[3].Stop();

    return 0;
}




TEST(api, raw) {
    test_rawapi(NULL);
}

