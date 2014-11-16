#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include <iostream>
#include "log.h"
#include "mq_stat_module.h"

using namespace std;

KMQ_DECLARATION_START

const char *mq_stat_module_item[MQ_MODULE_STATITEM_KEYRANGE] = {
    "",
    "cap",
    "size",
    "passed",
    "dropped",
    "max_msec",
    "min_msec",
    "all_msec",
    "avg_msec",
    "max_to_msec",
    "min_to_msec",
    "all_to_msec",
    "avg_to_msec",
};

__mq_stat_module_trigger::__mq_stat_module_trigger() :
    _s_trigger_cnt(0), _m_trigger_cnt(0), _h_trigger_cnt(0), _d_trigger_cnt(0)
{

}


__mq_stat_module_trigger::~__mq_stat_module_trigger() {

}

int __mq_stat_module_trigger::setup(const string &appid, const string &id) {
    _appid = appid;
    _rid = id;
    return 0;
}


#define __check_key_range(__key)					\
    do {								\
	if (__key <= 0 || __key > MQ_MODULE_STATITEM_KEYRANGE) {	\
	    KMQLOG_ERROR("module_stat invalid key %d", __key);	\
	    errno = EINVAL;						\
	    return -1;							\
	}								\
    } while (0)

int __mq_stat_module_trigger::trigger_s_threshold(module_stat *self,
						    int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat s_trigger appctx:" << _appid << " mq:" << _rid;
    ss << " [" << mq_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}

int __mq_stat_module_trigger::trigger_m_threshold(module_stat *self,
						    int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat m_trigger appctx:" << _appid << " mq:" << _rid;
    ss << " [" << mq_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}


int __mq_stat_module_trigger::trigger_h_threshold(module_stat *self,
						    int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat h_trigger appctx:" << _appid << " mq:" << _rid;
    ss << " [" << mq_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}

int __mq_stat_module_trigger::trigger_d_threshold(module_stat *self,
						    int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat d_trigger appctx:" << _appid << " mq:" << _rid;
    ss << " [" << mq_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}



int init_mq_stat_module_trigger(module_stat *qstat, const string &tl) {
    int tr = 0, val = 0;
    qstat->batch_unset_threshold();
    if (generic_trigger_item_parse(tl, "CAP", tr, val) == 0)
	generic_set_trigger_threshold(qstat, CAP, tr, val);
    if (generic_trigger_item_parse(tl, "SIZE", tr, val) == 0)
	generic_set_trigger_threshold(qstat, SIZE, tr, val);
    if (generic_trigger_item_parse(tl, "PASSED", tr, val) == 0)
	generic_set_trigger_threshold(qstat, PASSED, tr, val);
    if (generic_trigger_item_parse(tl, "DROPPED", tr, val) == 0)
	generic_set_trigger_threshold(qstat, DROPPED, tr, val);
    if (generic_trigger_item_parse(tl, "MAX_MSEC", tr, val) == 0)
	generic_set_trigger_threshold(qstat, MAX_MSEC, tr, val);
    if (generic_trigger_item_parse(tl, "MIN_MSEC", tr, val) == 0)
	generic_set_trigger_threshold(qstat, MIN_MSEC, tr, val);
    if (generic_trigger_item_parse(tl, "ALL_MSEC", tr, val) == 0)
	generic_set_trigger_threshold(qstat, ALL_MSEC, tr, val);
    if (generic_trigger_item_parse(tl, "AVG_MSEC", tr, val) == 0)
	generic_set_trigger_threshold(qstat, AVG_MSEC, tr, val);
    if (generic_trigger_item_parse(tl, "MAX_TO_MSEC", tr, val) == 0)
	generic_set_trigger_threshold(qstat, MAX_TO_MSEC, tr, val);
    if (generic_trigger_item_parse(tl, "MIN_TO_MSEC", tr, val) == 0)
	generic_set_trigger_threshold(qstat, MIN_TO_MSEC, tr, val);
    if (generic_trigger_item_parse(tl, "ALL_TO_MSEC", tr, val) == 0)
	generic_set_trigger_threshold(qstat, ALL_TO_MSEC, tr, val);
    if (generic_trigger_item_parse(tl, "AVG_TO_MSEC", tr, val) == 0)
	generic_set_trigger_threshold(qstat, AVG_TO_MSEC, tr, val);
    return 0;
}






}
