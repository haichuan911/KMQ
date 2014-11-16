#ifndef _H_LBFAIR_
#define _H_LBFAIR_

#include "load_balance.h"

KMQ_DECLARATION_START


class lb_fair : public LBAdapter {
 public:
    lb_fair();
    ~lb_fair();

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




#endif  // _H_LBFAIR_
