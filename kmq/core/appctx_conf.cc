#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "mem_status.h"
#include "appctx.h"


KMQ_DECLARATION_START

pthread_key_t appctx_global_conf;


int AppCtx::getconf(AppCfg &app_conf) {
    app_conf = conf;
    return 0;
}

static int __update_role_trigger_level(Role *r, void *data) {
    AppCfg *conf = (AppCfg *)data;
    MQueue *mq = r->Queue();

    init_mq_stat_module_trigger(mq->Stat(), conf->mq_trigger_level);
    init_role_stat_module_trigger(r->Stat(), conf->role_trigger_level);
    return 0;
}



int AppCtx::process_conf_update() {
    string roleid;
    AppCfg __nconf;
    map<string, string>::iterator it;

    tmutex_lock(&conf_lock);
    __nconf = nconf;
    tmutex_unlock(&conf_lock);
    if (!__nconf.mtime || __nconf.mtime <= conf.mtime)
	return 0;
    KMQLOG_NOTICE("app: %s update config", cid());
    conf.mtime = __nconf.mtime;
    
#define __update_app_config(field)		\
    if (__nconf.field > 0)			\
	conf.field = __nconf.field;

    __update_app_config(msg_queue_size);
    __update_app_config(msg_balance_factor);
    __update_app_config(msg_timeout_msec);
    __update_app_config(epoll_timeout_msec);
    __update_app_config(dumpstat_intern_msec);
    __update_app_config(reconnect_timeout_msec);
#undef __update_app_config

    {
	struct rom_conf romconf = {};
	romconf.timewait_msec = conf.reconnect_timeout_msec;
	rom.Update(&romconf);
    }

    // close some removed other apps connect
#define __remove_connectto_app(t)					\
    for (it = conf.t##_apps.begin(); it != conf.t##_apps.end(); ) {	\
	if (__nconf.raddr_exist(it->first) == true) {			\
	    it++;							\
	    continue;							\
	}								\
	roleid = it->second;						\
	conf.insert_removed(roleid);					\
	conf.at_apps.erase(it++);					\
    }

    __remove_connectto_app(at);
    __remove_connectto_app(wt);
    __remove_connectto_app(inat);
#undef __remove_connectto_app

    process_removed_app();

    // conf.at_apps + conf.wt_apps;
    for (it = __nconf.inat_apps.begin(); it != __nconf.inat_apps.end(); ++it) {
	if (conf.raddr_exist(it->first) == true)
	    continue;
	conf.insert_inactive(it->first, it->second);
    }
    if (__nconf.app_trigger_level != conf.app_trigger_level) {
	init_appctx_stat_module_trigger(&astat, __nconf.app_trigger_level);
	__nconf.app_trigger_level = conf.app_trigger_level;
    }

    if (__nconf.role_trigger_level != conf.role_trigger_level
	|| __nconf.mq_trigger_level != conf.mq_trigger_level) {
	rom.Walk(__update_role_trigger_level, &__nconf);
	__nconf.role_trigger_level = conf.role_trigger_level;
	__nconf.mq_trigger_level = conf.mq_trigger_level;
    }
    return 0;
}
    











}
