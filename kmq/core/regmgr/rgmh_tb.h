#ifndef _H_RGMMH_
#define _H_RGMMH_


#include "regmgr.h"
#include "pmutex.h"

KMQ_DECLARATION_START

typedef int (*rgm_walkfn) (RgmHandler *rgmh, void *data);

class rgmh_tb {
 public:
    rgmh_tb();
    ~rgmh_tb();

    int insert(RgmHandler *rgmh);
    int remove(RgmHandler *rgmh);
    bool exist(const string &appname);
    RgmHandler *balance_find(const struct kmqreg *rgh);
    int walk(rgm_walkfn walkfn, void *data);
    int walkone(const string &appname, rgm_walkfn walkfn, void *data);

 private:
    pmutex_t lock;
    map<string, int> _head_cnt;
    map<string, struct list_head *> _head;

    // no lock version
    int _walkone(const string &appname, rgm_walkfn walkfn, void *data);
};


}
 
#endif   // _H_RGMMH_
