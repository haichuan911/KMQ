#ifndef _H_LOAD_BALANCE_
#define _H_LOAD_BALANCE_

#include "role.h"
#include "proto.h"


KMQ_DECLARATION_START


enum BALANCE_ALGO {
    LB_RRBIN = 0,
    LB_RANDOM,
    LB_IPHASH,
    LB_FAIR,
    BALANCE_ALGO_KEYRANGE,
};

class LBAdapter {
 public:
    virtual ~LBAdapter() {}
    virtual int size() = 0;
    virtual int balance() = 0;
    virtual int add(Role *r) = 0;
    virtual int del(Role *r) = 0;
    virtual Role *loadbalance_recv() = 0;
    virtual Role *loadbalance_send(struct kmqmsg *msg) = 0;
};


LBAdapter *MakeLBAdapter(int lbalgo, ...);

}


#endif // _H_LOAD_BALANCE_
