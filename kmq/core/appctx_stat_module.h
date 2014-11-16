#ifndef _H_APPCTX_STAT_MODULE_
#define _H_APPCTX_STAT_MODULE_

#include "module_stats.h"

KMQ_DECLARATION_START


enum APP_MODULE_STATITEM {
    POLLRIN = 1,
    POLLROUT,
    POLLDIN,
    POLLDOUT,
    RRCVPKG,
    RSNDPKG,
    DRCVPKG,
    DSNDPKG,
    APP_MODULE_STATITEM_KEYRANGE,
};



int init_appctx_stat_module_trigger(module_stat *astat, const string &tl);

class __appctx_stat_module_trigger : public stat_module_trigger {
public:
    __appctx_stat_module_trigger();
    ~__appctx_stat_module_trigger();

    int setup(const string &appid);
    int trigger_s_threshold(module_stat *self, int key, int64_t threshold, int64_t val);
    int trigger_m_threshold(module_stat *self, int key, int64_t threshold, int64_t val);
    int trigger_h_threshold(module_stat *self, int key, int64_t threshold, int64_t val);
    int trigger_d_threshold(module_stat *self, int key, int64_t threshold, int64_t val);

 private:
    string id;
    int64_t _s_trigger_cnt;
    int64_t _m_trigger_cnt;
    int64_t _h_trigger_cnt;
    int64_t _d_trigger_cnt;
};



}
#endif  // _H_APPCTX_STAT_MODULE_
