#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include <monitor/client/client_agent.h>
#include "log.h"
#include "os.h"
#include "appctx.h"

using namespace std;
using namespace kmq;

extern SpioConfig kmq_conf;
appctx_mh *monitor_client = NULL;

class my_appctx_monitor : public appctx_mh {
public:
    my_appctx_monitor();
    ~my_appctx_monitor();
    int init(string &monitor_conf);
    int current_appctx_status(const string &appname, module_stat *astat);

private:
    int inited;
    monitor::ClientAgent *intern_monitor;
};


my_appctx_monitor::my_appctx_monitor() :
    inited(0), intern_monitor(NULL)
{

}

my_appctx_monitor::~my_appctx_monitor() {
    if (intern_monitor)
	delete intern_monitor;
}

int my_appctx_monitor::init(string &monitor_conf) {
    monitor::ClientAgent *__monitor = NULL;

    if (inited) {
	errno = KMQ_EINTERN;
	return -1;
    }
    __monitor = new (std::nothrow) monitor::ClientAgent();
    if (!__monitor) {
	errno = ENOMEM;
	return -1;
    }
    if (__monitor->Init(monitor_conf) < 0) {
	delete __monitor;
	return -1;
    }
    intern_monitor = __monitor;
    inited = 1;
    return 0;
}

int my_appctx_monitor::current_appctx_status(const string &appname, module_stat *astat) {
    string req_key, resp_key;
    string pin_key, pout_key;
    int64_t rcv_packages = 0, snd_packages = 0;
    
    req_key = appname + "_reqnums";
    resp_key = appname + "_respnums";
    
    rcv_packages = astat->getkey_s(RRCVPKG) + astat->getkey_s(DRCVPKG);
    snd_packages = astat->getkey_s(RSNDPKG) + astat->getkey_s(DSNDPKG);
    intern_monitor->Add(req_key, rcv_packages);
    intern_monitor->Add(resp_key, snd_packages);
    return 0;
}


int kmqd_start_monitor() {
    pid_t pid = gettid();
    my_appctx_monitor *mymonitor_client = NULL;

    mymonitor_client = new (std::nothrow) my_appctx_monitor();
    if (!mymonitor_client || mymonitor_client->init(kmq_conf.monitor) < 0) {
	KMQLOG_NOTICE("monitor start error with errno %d", errno);
	delete mymonitor_client;
	mymonitor_client = NULL;
	return -1;
    }
    monitor_client = mymonitor_client;
    KMQLOG_NOTICE("monitor start ok with pid %d", pid);
    return 0;
}


int kmqd_stop_monitor() {
    pid_t pid = gettid();

    if (monitor_client) {
	KMQLOG_NOTICE("monitor stop ok with pid %d", pid);
	delete monitor_client;
    }
    monitor_client = NULL;
    return 0;
}
