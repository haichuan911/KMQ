#ifndef _H_CONFIG_
#define _H_CONFIG_

#include <map>
#include <set>
#include "decr.h"

using namespace std;
KMQ_DECLARATION_START

enum {
    default_epoll_timeout_msec = 10,
    default_registe_timeout_msec = 5000,
    default_registe_timer_cap = 1024,
    default_registe_interval_msec = 10000,
    default_appconfupdate_interval_sec = 31536000, // one year
};

class SpioConfig {
 public:
    SpioConfig();
    ~SpioConfig();

    int Init(const char *conf);
    void Reset();

    string log4conf;
    string monitor;
    string apps_configdir;
    int epoll_timeout_msec;
    int registe_timeout_msec;
    int registe_timer_cap;
    int registe_interval_msec;
    int appconfupdate_interval_sec;
    set<string> regmgr_listen_addrs;
};


enum {
    default_pollcycle_count = 5,    
    default_max_cpu_core = 1,
    default_role_timeout_msec = 0,
    default_role_healthcheck_msec = 1000,
    default_msg_timeout_msec = 100,
    default_msg_balance_factor = 50,
    default_msg_balance_algo = 0,
    default_msg_queue_size = 1024000,
    default_msg_max_size = 0,
    default_mq_delay_die_msec = 60000,
    default_epoll_io_events = 500,
    default_reconnect_timeout_msec = 60000,
    default_dumpstat_intern_msec = 1000,
};

class AppCfg {
 public:
    AppCfg();
    ~AppCfg();
    
    int Init(const char *conf);
    bool raddr_exist(const string &raddr);
    int clone(AppCfg *cp);

    // Be careful iterator out of here
    int insert_inactive(const string &raddr, const string &id);
    int delete_inactive(const string &raddr);
    int insert_active(const string &raddr, const string &id);
    int delete_active(const string &raddr);
    int insert_waiting(const string &raddr, const string &id);
    int delete_waiting(const string &raddr);
    int insert_removed(const string &raddr);
    int delete_removed(const string &raddr);

    
    int64_t mtime;
    string appid;
    int pollcycle_count;
    int max_cpu_core;
    int role_timeout_msec;
    int role_healthcheck_msec;
    int msg_max_size;
    int msg_timeout_msec;
    int msg_balance_factor;
    int msg_balance_algo;
    int msg_queue_size;
    int epoll_io_events;
    int epoll_timeout_msec;
    int reconnect_timeout_msec;
    int dumpstat_intern_msec;
    string app_trigger_level;
    string mq_trigger_level;
    string role_trigger_level;
    set<string> rm_apps;
    map<string, string> inat_apps, at_apps, wt_apps;
};


struct rom_conf {
    int timewait_msec;
};
    

struct mpoller_conf {
    int max_io_events;
    int epoll_to_msec;
};


}










#endif // _H_AGCONFIG_
