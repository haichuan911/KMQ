#ifndef _H_ROLEMANAGER_H
#define _H_ROLEMANAGER_H


#include "pmutex.h"
#include "role.h"
#include "regmgr/regmgr.h"

KMQ_DECLARATION_START

typedef int (*rwalkfn) (Role *r, void *data);

class RoleManager : public RgmHandler {
 public:
    RoleManager();
    ~RoleManager();

    inline const char *cappid() {
	return appid.c_str();
    }
    inline void Init(const string &_appid, struct rom_conf *nconf) {
	appid = _appid;
	if (nconf)
	    conf = *nconf;
    }
    inline void Update(struct rom_conf *nconf) {
	if (nconf)
	    conf = *nconf;
    }
    int stats(struct rgmh_stats *info);
    Role *PopNewer();
    Role *FindRole(const string &id);
    int Walk(rwalkfn walkfn, void *data);
    int WalkReceivers(rwalkfn walkfn, void *data);
    int WalkDispatchers(rwalkfn walkfn, void *data);
    int TimeWait(Role *r);
    bool reg_keep_session(const struct kmqreg *rgh);
    int Register(struct kmqreg *rgh, Conn *conn);
    int OutputRoleStatus(FILE *fp);
    
 private:
    string appid;
    pmutex lock;
    inline int biglock();
    inline int unbiglock();

    struct rom_conf conf;
    struct rgmh_stats romstat;
    map<string, Role *> active_roles;
    struct list_head receivers, dispatchers;

    int __walk_receivers(rwalkfn walkfn, void *data);
    int __walk_dispatchers(rwalkfn walkfn, void *data);
    
    struct list_head newroles;
    inline Role *pop_new_role();
    inline Role *find_new_role(const string roleid, uint32_t rtyp);
    inline void push_new_role(Role *r);

    struct list_head tw_receivers;
    struct list_head tw_dispatchers;
    rbtree_t tw_tree;
    rbtree_node_t tw_sentinel;
    inline void clean_time_wait_roles();
    inline Role *__find_tw_r(const string rid, struct list_head *head);
    inline Role *find_time_wait_role(const string roleid, uint32_t rtyp);
    inline void insert_time_wait_role(Role *r);
};




}


#endif    // _H_ROLEMANAGER_H
