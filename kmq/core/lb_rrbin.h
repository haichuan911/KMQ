#ifndef _H_LB_RRBIN_
#define _H_LB_RRBIN_


#include "load_balance.h"


KMQ_DECLARATION_START

class lb_rrbin : public LBAdapter {
 public:
    lb_rrbin();
    ~lb_rrbin();

    int add(Role *r);
    int del(Role *r);
    int size();
    int balance();
    Role *loadbalance_recv();
    Role *loadbalance_send(struct kmqmsg *msg);
    
 private:
    int idx, numbers, cap;
    Role **backend_servers;

    Role *__loadbalance_send(struct kmqmsg *msg);
};


}













#endif   // _H_LB_RRBIN_
