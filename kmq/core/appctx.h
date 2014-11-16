#ifndef _H_APPCONTEX_
#define _H_APPCONTEX_


#include "tmutex.h"
#include "osthread.h"
#include "load_balance.h"
#include "config.h"
#include "mqp.h"
#include "regmgr/regmgr.h"
#include "rolemgr.h"
#include "role_attr_evh.h"
#include "appctx_stat_module.h"

using namespace std;

KMQ_DECLARATION_START

#define list_app(link) ((AppCtx *)link->self) 
#define list_first_app(head)						\
    ({struct list_link *__pos =						\
	    list_first(head, struct list_link, node);  list_app(__pos);})

class appctx_mh {
 public:
    virtual ~appctx_mh() {}
    virtual int current_appctx_status(const string &appname, module_stat *astat) = 0;
};


class AppCtx {
 public:
    AppCtx();
    ~AppCtx();

    string Id();
    int SetDebugMode(int mode);
    int SetMonitor(appctx_mh *monitor_handler);
    int Init(const string &id);
    int InitFromConf(AppCfg *app_conf);
    int Update(AppCfg *app_conf);
    int EnableRegistry(Rgm *_rgm);
    int Running();
    int Stop();
    int Start();
    int StopThread();
    int StartThread();

    int getconf(AppCfg &app_conf);
    int attach_to_apps_head(struct list_head *head);
    int detach_from_apps_head();
    
 private:
    bool stopping;
    uint32_t attr;
    int64_t now_time;
    
    // only update when:
    //   1. the first time has no dispatchers
    //   2. when the first dispatcher is registered
    int64_t first_no_dispatchers;
    int debugmode;
    int inited;
    string appid;
    OSThread thread;
    tmutex_t conf_lock; // locking nconf
    AppCfg conf, nconf;
    LBAdapter *lbp;
    MQueuePool mqp;
    RoleManager rom;
    rattr_ev_monitor ratm;
    Epoller *poller;
    Rgm *rgm;
    struct list_link apps_link;
    
    inline const char *cid() {
	return appid.c_str();
    }

    appctx_mh *monitor;
    __appctx_stat_module_trigger __appctx_sm;
    module_stat astat;

    // loadbalance recv an send
    int receiver_recv(Role *r);
    int receiver_send(Role *r);
    int dispatcher_recv(Role *r);
    int dispatcher_send(Role *r);

    struct list_head err_roles;
    int wait_massage(struct list_head *rin, struct list_head *rout,
		     struct list_head *din, struct list_head *dout);
    int recv_massage(struct list_head *rhead, struct list_head *dhead);
    int sendto_massage(struct list_head *rhead, struct list_head *dhead);

    int process_conf_update();
    int process_console_show(int fd);
    int process_massage();

    int process_removed_app();
    int process_connectto_app();
    bool is_time_to_close_all_receivers();
    int update_all_roles_status();
    int process_roles_register();
    int process_roles_error();
    int init_r_stat_module_trigger_level(Role *r);
    int process_single_error(Role *r);
    int broadcast_role_status_icmp();
};







}







#endif  // _H_APPGROUP_
