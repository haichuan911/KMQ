#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <kmq/errno.h>
#include "rid.h"
#include "path.h"
#include "mem_status.h"
#include "config.h"
#include "ini_parser.h"


KMQ_DECLARATION_START

extern kmq_mem_status_t kmq_mem_stats;

static mem_stat_t *kmqconfig_mem_stats = &kmq_mem_stats.kmq_config;
static mem_stat_t *appconfig_mem_stats = &kmq_mem_stats.app_config;

#define __kmqconf_int_parse(sec, field)	\
    do {					\
	field = parser.get_int(sec, #field);	\
	if (field == 0)				\
	    field = default_##field;		\
    } while (0)

#define __kmqconf_string_parse(sec, field)		\
    do {						\
	if (!(p = parser.get_string(sec, #field))) {	\
	    errno = KMQ_ECONFIGURE;			\
	    return -1;					\
	}						\
	field = p;					\
    } while (0)

#define __kmqconf_filepath_parse(sec, field)	\
    do {					\
	__kmqconf_string_parse(sec, field);	\
	if (!IsAbs(field))			\
	    field = Abs(dir + field);		\
	if (0 != access(field.c_str(), F_OK)) {	\
	    errno = KMQ_ECONFIGURE;		\
	    return -1;				\
	}					\
    } while (0)


    
SpioConfig::SpioConfig() :

    epoll_timeout_msec(default_epoll_timeout_msec),
    registe_timeout_msec(default_registe_timeout_msec),
    registe_timer_cap(default_registe_timer_cap),
    registe_interval_msec(default_registe_interval_msec),
    appconfupdate_interval_sec(default_appconfupdate_interval_sec)
{
    kmqconfig_mem_stats->alloc++;
    kmqconfig_mem_stats->alloc_size += sizeof(SpioConfig);
}

SpioConfig::~SpioConfig() {
    kmqconfig_mem_stats->alloc--;
    kmqconfig_mem_stats->alloc_size -= sizeof(SpioConfig);
}

void SpioConfig::Reset() {
    apps_configdir = log4conf = "";
    registe_timer_cap = default_registe_timer_cap;
    registe_timeout_msec = default_registe_timeout_msec;
    registe_interval_msec = default_registe_interval_msec;
    epoll_timeout_msec = default_epoll_timeout_msec;
    appconfupdate_interval_sec = default_appconfupdate_interval_sec;
    regmgr_listen_addrs.clear();
}    

int SpioConfig::Init(const char *conf) {
    ini_parser_t parser;
    const char *p;
    string dir, laddr;
    char *listen;

    if (0 != access(conf, F_OK))
	return -1;
    dir = Dir(conf);
    parser.load(conf);

    __kmqconf_filepath_parse("kmq", log4conf);
    __kmqconf_filepath_parse("kmq", monitor);
    __kmqconf_filepath_parse("kmq", apps_configdir);

    // register manager listen addrs
    if (!(p = parser.get_string("kmq", "regmgr_listen_addrs"))) {
	errno = KMQ_ECONFIGURE;
	return -1;
    }
    listen = strdup(p);
    p = strtok(listen, ";");
    while (p) {
	laddr = p;
	regmgr_listen_addrs.insert(laddr);
	p = strtok(NULL, ";");
    }
    free(listen);

    __kmqconf_int_parse("kmq", epoll_timeout_msec);
    __kmqconf_int_parse("kmq", registe_timeout_msec);
    __kmqconf_int_parse("kmq", registe_timer_cap);
    __kmqconf_int_parse("kmq", registe_interval_msec);
    __kmqconf_int_parse("kmq", appconfupdate_interval_sec);
    
    return 0;
}


AppCfg::AppCfg() :
    mtime(0),
    pollcycle_count(default_pollcycle_count),
    max_cpu_core(default_max_cpu_core),
    role_timeout_msec(default_role_timeout_msec),
    role_healthcheck_msec(default_role_healthcheck_msec),
    msg_max_size(default_msg_max_size),
    msg_timeout_msec(default_msg_timeout_msec),
    msg_balance_factor(default_msg_balance_factor),
    msg_balance_algo(default_msg_balance_algo),
    msg_queue_size(default_msg_queue_size),
    epoll_io_events(default_epoll_io_events),
    epoll_timeout_msec(default_epoll_timeout_msec),
    reconnect_timeout_msec(default_reconnect_timeout_msec),
    dumpstat_intern_msec(default_dumpstat_intern_msec)
{
    appconfig_mem_stats->alloc++;
    appconfig_mem_stats->alloc_size += sizeof(SpioConfig);
}


AppCfg::~AppCfg() {
    appconfig_mem_stats->alloc--;
    appconfig_mem_stats->alloc_size -= sizeof(SpioConfig);
}



int AppCfg::Init(const char *conf) {
    ini_parser_t parser;
    const char *p;
    string raddr, roleid;
    char *connect_to, *cur;
    struct stat finfo = {};

    if (stat(conf, &finfo) < 0)
	return -1;
    parser.load(conf);

    __kmqconf_string_parse("app", appid);
    __kmqconf_int_parse("app", pollcycle_count);
    __kmqconf_int_parse("app", max_cpu_core);
    __kmqconf_int_parse("app", role_timeout_msec);
    __kmqconf_int_parse("app", msg_max_size);
    __kmqconf_int_parse("app", msg_timeout_msec);
    __kmqconf_int_parse("app", msg_balance_factor);
    __kmqconf_int_parse("app", msg_balance_algo);
    __kmqconf_int_parse("app", msg_queue_size);
    __kmqconf_int_parse("app", epoll_io_events);    
    __kmqconf_int_parse("app", epoll_timeout_msec);
    __kmqconf_int_parse("app", reconnect_timeout_msec);
    __kmqconf_int_parse("app", dumpstat_intern_msec);
    
    if ((p = parser.get_string("app", "connect_to_apps")) != NULL) {
	connect_to = strdup(p);
	cur = strtok(connect_to, ";");
	while (cur) {
	    raddr = cur;
	    route_genid(roleid);
	    inat_apps.insert(make_pair(raddr, roleid));
	    cur = strtok(NULL, ";");
	}
	free(connect_to);
    }
    __kmqconf_string_parse("module_stat", app_trigger_level);
    __kmqconf_string_parse("module_stat", mq_trigger_level);
    __kmqconf_string_parse("module_stat", role_trigger_level);
    mtime = finfo.st_mtime;

    return 0;
}

#undef __kmqconf_int_parse
#undef __kmqconf_filepath_parse


bool AppCfg::raddr_exist(const string &raddr) {
    map<string, string>::iterator it;

    if ((it = at_apps.find(raddr)) != at_apps.end())
	return true;
    if ((it = inat_apps.find(raddr)) != inat_apps.end())
	return true;
    if ((it = wt_apps.find(raddr)) !=
	wt_apps.end())
	return true;
    return false;
}    

int AppCfg::insert_inactive(const string &raddr, const string &id) {
    inat_apps.insert(make_pair(raddr, id));
    return 0;
}

int AppCfg::delete_inactive(const string &raddr) {
    map<string, string>::iterator it;

    if ((it = inat_apps.find(raddr)) != inat_apps.end())
	inat_apps.erase(it);
    return 0;
}

int AppCfg::insert_active(const string &raddr, const string &id) {
    at_apps.insert(make_pair(raddr, id));
    return 0;
}

int AppCfg::delete_active(const string &raddr) {
    map<string, string>::iterator it;

    if ((it = at_apps.find(raddr)) != at_apps.end())
	at_apps.erase(it);
    return 0;
}

int AppCfg::insert_waiting(const string &raddr, const string &id) {
    wt_apps.insert(make_pair(raddr, id));
    return 0;
}

int AppCfg::delete_waiting(const string &raddr) {
    map<string, string>::iterator it;

    if ((it = wt_apps.find(raddr)) != wt_apps.end())
	wt_apps.erase(it);
    return 0;
}

int AppCfg::insert_removed(const string &raddr) {
    rm_apps.insert(raddr);
    return 0;
}

int AppCfg::delete_removed(const string &raddr) {
    set<string>::iterator it;

    if ((it = rm_apps.find(raddr)) != rm_apps.end())
	rm_apps.erase(it);
    return 0;
}



int AppCfg::clone(AppCfg *cp) {
    string roleid;
    map<string, string>::iterator it;
    
    *cp = *this;
    cp->inat_apps.clear();
    for (it = inat_apps.begin(); it != inat_apps.end(); ++it) {
	route_genid(roleid);
	cp->inat_apps.insert(make_pair(it->first, roleid));
    }
    return 0;
}

    
}
