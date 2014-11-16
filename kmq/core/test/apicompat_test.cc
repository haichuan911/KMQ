#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include <kmq/errno.h>
#include <kmq/compatapi.h>
#include "CRC.h"
#include "osthread.h"
#include "appctx.h"

using namespace kmq;


#define buflen 1024
static char buffer[buflen] = {};
static string appname = "testapp";
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
static int cnt = 20;

// test compatapi for CSpioApi
// send(const string &msg, int timeout);
// recv(string &msg, int timeout);

static volatile int s1_status = 0;
static int app_server1(void *arg_) {
    CSpioApi app;
    int i;

    while (!agstarted || !rmstarted)
	usleep(10);
    app.init(appname);
    if (app.join_server("127.0.0.1:20010") < 0)
	return -1;
    s1_status = 1;
    for (i = 0; i < cnt; i++) {
	string msg;
	EXPECT_TRUE(app.recv(msg, 1000) == 0);
	EXPECT_TRUE(app.send(msg, 1000) == 0);
    }
    app.terminate();
    s1_status = 0;
    return 0;
}


static int app_client1(void *arg_) {
    CSpioApi app;
    int i;

    while (!agstarted || !rmstarted || !s1_status)
	usleep(10);
    app.init(appname);
    app.join_client("127.0.0.1:20010");
    for (i = 0; i < cnt; i++) {
	string req, resp;
	EXPECT_TRUE(app.send(req, 1000) == 0);
	EXPECT_TRUE(app.recv(resp, 1000) == 0);
	EXPECT_TRUE(req == resp);
    }
    app.terminate();
    return 0;
}


// test ramdome msg
static volatile int s2_status = 0;
static int app_server2(void *arg_) {
    CSpioApi app;
    int i;

    while (!agstarted || !rmstarted)
	usleep(10);
    app.init(appname);
    if (app.join_server("127.0.0.1:20010") < 0)
	return -1;
    s2_status = 1;
    for (i = 0; i < cnt; i++) {
	string msg;
	EXPECT_TRUE(app.recv(msg, 1000) == 0);
	EXPECT_TRUE(app.send(msg, 1000) == 0);
    }
    app.terminate();
    s2_status = 0;
    return 0;
}


static int app_client2(void *arg_) {
    CSpioApi app;
    int i;

    while (!agstarted || !rmstarted || !s2_status)
	usleep(10);

    app.init(appname);
    app.join_client("127.0.0.1:20010");
    for (i = 0; i < cnt; i++) {
	string req, resp;
	req.assign(buffer, rand() % buflen);
	EXPECT_TRUE(app.send(req, 1000) == 0);
	EXPECT_TRUE(app.recv(resp, 1000) == 0);
	EXPECT_TRUE(req == resp);
    }
    app.terminate();
    return 0;
}


// test app_client timeout
static volatile int s3_status = 0;
static int app_server3(void *arg_) {
    CSpioApi app;
    string msg;

    while (!agstarted || !rmstarted)
	usleep(10);
    app.init(appname);
    if (app.join_server("127.0.0.1:20010") < 0)
	return -1;
    s3_status = 1;
    EXPECT_TRUE(app.recv(msg, 1000) == 0);
    app.terminate();
    s3_status = 0;
    return 0;
}


    

static int app_client3(void *arg_) {
    CSpioApi app;
    string req, resp;
    
    while (!agstarted || !rmstarted || !s3_status)
	usleep(10);

    app.init(appname);
    app.join_client("127.0.0.1:20010");
    req.assign(buffer, rand() % buflen);
    EXPECT_TRUE(app.send(req, 1) == 0);
    EXPECT_TRUE(app.recv(resp, 1) == -1 && errno == EAGAIN);
    app.terminate();
    return 0;
}


// test app_server timeout
static volatile int s4_status = 0;
static int app_server4(void *arg_) {
    CSpioApi app;
    string msg;

    while (!agstarted || !rmstarted)
	usleep(10);
    app.init(appname);
    if (app.join_server("127.0.0.1:20010") < 0)
	return -1;
    s4_status = 1;
    EXPECT_TRUE(app.recv(msg, 1000) == 0);
    EXPECT_TRUE(app.recv(msg, 1) == -1 && errno == EAGAIN);
    app.terminate();
    s4_status = 0;
    return 0;
}

static int app_client4(void *arg_) {
    CSpioApi app;
    string req, resp;
    
    while (!agstarted || !rmstarted || !s4_status)
	usleep(10);

    app.init(appname);
    app.join_client("127.0.0.1:20010");
    req.assign(buffer, rand() % buflen);
    usleep(10);
    EXPECT_TRUE(app.send(req, 1000) == 0);
    EXPECT_TRUE(app.recv(resp, 1) == -1 && errno == EAGAIN);
    app.terminate();
    return 0;
}


// test kmqdown timeout
static volatile int s5_started = 0;
static volatile int c5_started = 0;
static volatile int ag_stopped = 0;
static volatile int s5_status = 0;
static int app_server5(void *arg_) {
    CSpioApi app;
    string msg;

    while (!agstarted || !rmstarted)
	usleep(10);
    app.init(appname);
    if (app.join_server("127.0.0.1:20010") < 0)
	return -1;
    s5_status = 1;
    EXPECT_TRUE(app.recv(msg, 1000) == 0);
    s5_started = 1;
    while (!ag_stopped)
	usleep(1);
    while (app.recv(msg, 1) != 0) {
	usleep(1);
    }
    EXPECT_TRUE(app.send(msg, 1000) == 0);
    s5_status = 0;
    app.terminate();
    return 0;
}

static int app_client5(void *arg_) {
    CSpioApi app;
    string req, resp;
    
    while (!agstarted || !rmstarted || !s5_status)
	usleep(10);

    app.init(appname);
    app.join_client("127.0.0.1:20010");
    req.assign(buffer, rand() % buflen);
    EXPECT_TRUE(app.send(req, 1000) == 0);
    c5_started = 1;
    while (!ag_stopped)
	usleep(1);
    while (app.send(req, 1) != 0) {
	usleep(1);
    }
    EXPECT_TRUE(app.recv(resp, 1000) == 0);
    app.terminate();
    return 0;
}


// test app_server timeout
static volatile int s6_status = 0;
static int app_server6(void *arg_) {
    CSpioApi app;
    string msg;

    while (!agstarted || !rmstarted)
	usleep(10);
    app.init(appname);
    if (app.join_server("127.0.0.1:20010") < 0)
	return -1;
    s6_status = 1;
    EXPECT_TRUE(app.recv(msg, 50) == 0);
    usleep(100000); // sleep 100ms
    msg.clear();
    EXPECT_TRUE(app.recv(msg, 1000) == 0);
    EXPECT_TRUE(app.send(msg, 1000) == 0);
    EXPECT_TRUE(app.recv(msg, 50) == -1 && errno == EAGAIN);
    app.terminate();
    s6_status = 0;
    return 0;
}

static int app_client6(void *arg_) {
    uint16_t checksum = 0;
    CSpioApi app;
    string req, resp;
    
    while (!agstarted || !rmstarted || !s6_status)
	usleep(10);

    app.init(appname);
    app.join_client("127.0.0.1:20010");
    req.assign(buffer, rand() % buflen);
    EXPECT_TRUE(app.send(req, 50) == 0);
    EXPECT_TRUE(app.recv(resp, 50) == -1 && errno == EAGAIN);
    req.assign(buffer, rand() % buflen);
    checksum = crc16(req.data(), req.size());
    EXPECT_TRUE(app.send(req, 50) == 0);
    EXPECT_TRUE(app.recv(resp, 200) == 0);
    EXPECT_TRUE(checksum == crc16(resp.data(), resp.size()));
    app.terminate();
    return 0;
}


static int test_compatapi(void *arg) {
    AppCtx *ag = new AppCtx();
    Rgm *rgm = NewRegisterManager(NULL);
    OSThread t[4];

    app_conf.appid = appname;
    ag->InitFromConf(&app_conf);
    ag->EnableRegistry(rgm);
    rgm->Listen("*:20010");
    rgm->StartThread();
    ag->StartThread();
    agstarted = rmstarted = 1;

    t[2].Start(app_server1, NULL);
    t[3].Start(app_client1, NULL);
    t[2].Stop();
    t[3].Stop();

    t[2].Start(app_server2, NULL);
    t[3].Start(app_client2, NULL);
    t[2].Stop();
    t[3].Stop();

    t[2].Start(app_server3, NULL);
    t[3].Start(app_client3, NULL);
    t[2].Stop();
    t[3].Stop();

    t[2].Start(app_server4, NULL);
    t[3].Start(app_client4, NULL);
    t[2].Stop();
    t[3].Stop();

    t[2].Start(app_server6, NULL);
    t[3].Start(app_client6, NULL);
    t[2].Stop();
    t[3].Stop();
    
    t[2].Start(app_server5, NULL);
    t[3].Start(app_client5, NULL);
    
    while (!c5_started || !s5_started)
	usleep(1);
    rgm->StopThread();
    ag->StopThread();
    delete ag;
    delete rgm;
    ag_stopped = 1;

    // ag restart
    rgm = NewRegisterManager(NULL);
    ag = new AppCtx();
    ag->InitFromConf(&app_conf);
    ag->EnableRegistry(rgm);
    rgm->Listen("*:20010");
    rgm->StartThread();
    ag->StartThread();

    t[2].Stop();
    t[3].Stop();

    rgm->StopThread();
    ag->StopThread();
    delete ag;    
    delete rgm;

    return 0;
}


TEST(api, compat) {
    randstr(buffer, buflen);
    test_compatapi(NULL);
}
