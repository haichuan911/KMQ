#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include "appctx.h"
#include "config.h"
#include "api.h"

using namespace kmq;

#define buflen 1024
static char buffer[buflen] = {};
static string appname = "testapp";
static string apphost = "127.0.0.1:20002";
static string apphost2 = "127.0.0.1:20003";
static volatile int agstarted = 0, rmstarted = 0;
static volatile int cnt = 100;


static int randstr(char *buf, int len) {
    int i, idx;
    char token[] = "qwertyuioplkjhgfdsazxcvbnm1234567890";
    for (i = 0; i < len; i++) {
	idx = rand() % strlen(token);
	buf[i] = token[idx];
    }
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
	    break;
	case 2:
	    server.Connect(appname, apphost2);
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
	switch (rand() % 3) {
	case 0:
	    if (sendcnt < cnt) {
		req.s.len = (rand() % buflen) + HDRLEN;
		req.s.data = buffer;
		if (client.SendMsg(&req) == 0)
		    sendcnt++;
	    }
	    break;
	case 1:
	    client.Close();
	    break;
	case 2:
	    client.Connect(appname, apphost);
	    client.SetOption(OPT_NONBLOCK, 1);
	    break;
	}
	if (client.RecvMsg(&resp) == 0) {
	    if (resp.s.len)
		free(resp.s.data);
	    recvcnt++;
	}
    }
    _stopping = 1;
    return 0;
}


static AppCfg app_conf;

static int test_appcontext(void *arg) {
    AppCtx *ag = new AppCtx();
    AppCtx *ag2 = new AppCtx();
    Rgm *rgm = NewRegisterManager(NULL);
    Rgm *rgm2 = NewRegisterManager(NULL);
    OSThread t[2];

    randstr(buffer, buflen);
    app_conf.appid = appname;
    app_conf.msg_timeout_msec = 50;
    ag->InitFromConf(&app_conf);
    ag2->InitFromConf(&app_conf);    
    ag2->EnableRegistry(rgm2);
    rgm2->Listen(apphost2);
    rgm2->StartThread();
    ag2->StartThread();

    ag->EnableRegistry(rgm);
    rgm->Listen(apphost);
    rgm->StartThread();
    ag->StartThread();
    rmstarted = 1;
    agstarted = 1;

    t[0].Start(app_server1, NULL);
    t[1].Start(app_client1, NULL);
    t[0].Stop();
    t[1].Stop();

    rgm->StopThread();
    rgm2->StopThread();
    rmstarted = 1;
    ag->StopThread();
    ag2->StopThread();
    agstarted = 1;
    delete ag;
    delete ag2;
    delete rgm;
    delete rgm2;
    return 0;
}


TEST(api, appcontext) {
    test_appcontext(NULL);
}
