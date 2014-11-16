#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "mem_status.h"
#include "appctx.h"


KMQ_DECLARATION_START

extern kmq_mem_status_t kmq_mem_stats;
static mem_stat_t *app_context_mem_stats = &kmq_mem_stats.app_context;

AppCtx::AppCtx() :
    stopping(true), attr(0), now_time(0), first_no_dispatchers(0),
    debugmode(0), inited(0), lbp(NULL), poller(NULL), rgm(NULL),
    monitor(NULL), astat(APP_MODULE_STATITEM_KEYRANGE, &__appctx_sm)
{
    tmutex_init(&conf_lock);
    INIT_LIST_HEAD(&err_roles);
    INIT_LIST_LINK(&apps_link);
    app_context_mem_stats->alloc++;
    app_context_mem_stats->alloc_size += sizeof(AppCtx);
}

AppCtx::~AppCtx() {
    tmutex_destroy(&conf_lock);
    if (poller)
	delete poller;
    if (rgm)
	rgm->RemoveHandler(&rom);
    if (lbp)
	delete lbp;
    app_context_mem_stats->alloc--;
    app_context_mem_stats->alloc_size -= sizeof(AppCtx);
}

int AppCtx::attach_to_apps_head(struct list_head *head) {
    if (!apps_link.linked) {
	apps_link.linked = 1;
	list_add(&apps_link.node, head);
	return 0;
    }
    return -1;
}

int AppCtx::detach_from_apps_head() {
    if (apps_link.linked) {
	list_del(&apps_link.node);
	apps_link.linked = 0;
	return 0;
    }
    return -1;
}

    
string AppCtx::Id() {
    return appid;
}

int AppCtx::SetDebugMode(int mode) {
    debugmode = mode ? 1 : 0;
    return 0;
}

int AppCtx::SetMonitor(appctx_mh *monitor_handler) {
    monitor = monitor_handler;
    return 0;
}

int AppCtx::Init(const string &id) {
    AppCfg default_conf;

    default_conf.appid = id;
    return InitFromConf(&default_conf);
}

int AppCtx::Update(AppCfg *app_conf) {
    tmutex_lock(&conf_lock);
    nconf = *app_conf;
    tmutex_unlock(&conf_lock);
    return 0;
}

int AppCtx::InitFromConf(AppCfg *app_conf) {

    if (inited) {
	KMQLOG_WARN("appid %s is inited", appid.c_str());
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!(poller = EpollCreate(10240, app_conf->epoll_io_events))) {
	KMQLOG_ERROR("poller init errno: %d", errno);
	return -1;
    }
    if (!(lbp = MakeLBAdapter(app_conf->msg_balance_algo))) {
	delete poller;
	KMQLOG_ERROR("make loadbalance adapter with errno %d", errno);
	return -1;
    }
    if (app_conf)
	conf = *app_conf;
    appid = conf.appid;
    __appctx_sm.setup(appid);
    init_appctx_stat_module_trigger(&astat, conf.app_trigger_level);
    {
	struct rom_conf romconf = {};
	romconf.timewait_msec = conf.reconnect_timeout_msec;
	rom.Init(appid, &romconf);
    }
    {
	struct mqp_conf mqpconf = {};
	mqpconf.mq_cap = conf.msg_queue_size;
	mqpconf.mq_timewait_msec = conf.reconnect_timeout_msec;
	mqpconf.msg_timeout_msec = conf.msg_timeout_msec;
	mqp.Setup(appid, &mqpconf);
    }
    inited = 1;
    return 0;
}


int AppCtx::Stop() {
    stopping = true;
    return 0;
}

int AppCtx::Start() {
    pid_t pid = gettid();
    int64_t start_tt = rt_mstime(), healthcheck = rt_mstime();

    stopping = false;
    first_no_dispatchers = start_tt;
    KMQLOG_NOTICE("app %s start ok with pid %d", cid(), pid);
    while (!stopping) {
	now_time = rt_mstime();
	astat.update_timestamp(now_time);

	process_massage();

	// Each conf.dumpstat_intern_msec time, flush stats
	if (now_time - start_tt > conf.dumpstat_intern_msec) {
	    if (debugmode) {
		system("clear");
		process_console_show(-1);
	    }
	    if (monitor)
		monitor->current_appctx_status(appid, &astat);
	    start_tt = now_time;

	}
	if (now_time - healthcheck > conf.role_healthcheck_msec) {
	    broadcast_role_status_icmp();
	    healthcheck = now_time;
	}
	if (nconf.mtime > conf.mtime)
	    process_conf_update();

	process_roles_register();
    }
    KMQLOG_NOTICE("app %s stop ok with pid %d", cid(), pid);

    return 0;
}


int AppCtx::Running() {
    return stopping ? false : true;
}

int AppCtx::StopThread() {
    bool __running = Running();
    Stop();
    if (__running)
	thread.Stop();
    return 0;
}

static int appctx_thread_worker(void *arg_) {
    AppCtx *ctx = (AppCtx *)arg_;
    return ctx->Start();
}
    
int AppCtx::StartThread() {
    if (stopping)
	thread.Start(appctx_thread_worker, this);
    return 0;
}    


}
