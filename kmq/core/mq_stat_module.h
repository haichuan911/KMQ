#ifndef _H_MQ_STAT_MODULE_
#define _H_MQ_STAT_MODULE_

#include "module_stats.h"

KMQ_DECLARATION_START


enum MQ_MODULE_STATITEM {
    CAP = 1,
    SIZE,
    PASSED,
    DROPPED,
    MAX_MSEC,
    MIN_MSEC,
    ALL_MSEC,
    AVG_MSEC,
    MAX_TO_MSEC,
    MIN_TO_MSEC,
    ALL_TO_MSEC,
    AVG_TO_MSEC,
    MQ_MODULE_STATITEM_KEYRANGE,
};

int init_mq_stat_module_trigger(module_stat *astat, const string &tl);

class __mq_stat_module_trigger : public stat_module_trigger {
public:
    __mq_stat_module_trigger();
    ~__mq_stat_module_trigger();

    int setup(const string &appid, const string &id);
    int trigger_s_threshold(module_stat *self, int key, int64_t threshold, int64_t val);
    int trigger_m_threshold(module_stat *self, int key, int64_t threshold, int64_t val);
    int trigger_h_threshold(module_stat *self, int key, int64_t threshold, int64_t val);
    int trigger_d_threshold(module_stat *self, int key, int64_t threshold, int64_t val);

 private:
    string _appid, _rid;
    int64_t _s_trigger_cnt;
    int64_t _m_trigger_cnt;
    int64_t _h_trigger_cnt;
    int64_t _d_trigger_cnt;
};



}
#endif  // _H_APPCTX_STAT_MODULE_
