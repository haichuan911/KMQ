#ifndef _H_LB_RANDOM_
#define _H_LB_RANDOM_


#include "load_balance.h"


KMQ_DECLARATION_START

class lb_random : public LBAdapter {
 public:
    lb_random();
    ~lb_random();

    int add(Role *r);
    int del(Role *r);
    int size();
    int balance();
    Role *loadbalance_recv();
    Role *loadbalance_send(struct kmqmsg *msg);

 private:
    int numbers, cap;
    Role **backend_servers;

    Role *__loadbalance_send(struct kmqmsg *msg);
};



}












#endif   // _H_LB_RANDOM_
