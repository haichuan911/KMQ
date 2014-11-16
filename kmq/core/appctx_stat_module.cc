#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include <iostream>
#include "log.h"
#include "appctx_stat_module.h"

using namespace std;

KMQ_DECLARATION_START

const char *app_stat_module_item[APP_MODULE_STATITEM_KEYRANGE] = {
    "",
    "pollrin",
    "pollrout",
    "polldin",
    "polldout",
    "rrcvpkg",
    "rsndpkg",
    "drcvpkg",
    "dsndpkg"
};

__appctx_stat_module_trigger::__appctx_stat_module_trigger() :
    _s_trigger_cnt(0), _m_trigger_cnt(0), _h_trigger_cnt(0), _d_trigger_cnt(0)
{

}


__appctx_stat_module_trigger::~__appctx_stat_module_trigger() {

}

int __appctx_stat_module_trigger::setup(const string &appid) {
    id = appid;
    return 0;
}

#define __check_key_range(__key)					\
    do {								\
	if (__key <= 0 || __key > APP_MODULE_STATITEM_KEYRANGE) {	\
	    KMQLOG_ERROR("module_stat invalid key %d", __key);	\
	    errno = EINVAL;						\
	    return -1;							\
	}								\
    } while (0)

int __appctx_stat_module_trigger::trigger_s_threshold(module_stat *self,
						      int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat s_trigger appctx:" << id;
    ss << "[" << app_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}

int __appctx_stat_module_trigger::trigger_m_threshold(module_stat *self,
						      int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat m_trigger appctx:" << id;
    ss << "[" << app_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}


int __appctx_stat_module_trigger::trigger_h_threshold(module_stat *self,
						      int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat h_trigger appctx:" << id;
    ss << "[" << app_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}

int __appctx_stat_module_trigger::trigger_d_threshold(module_stat *self,
						      int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat d_trigger appctx:" << id;
    ss << "[" << app_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}


int init_appctx_stat_module_trigger(module_stat *astat, const string &tl) {
    int tr = 0, val = 0;
    astat->batch_unset_threshold();
    if (generic_trigger_item_parse(tl, "POLLRIN", tr, val) == 0)
	generic_set_trigger_threshold(astat, POLLRIN, tr, val);
    if (generic_trigger_item_parse(tl, "POLLROUT", tr, val) == 0)
	generic_set_trigger_threshold(astat, POLLROUT, tr, val);
    if (generic_trigger_item_parse(tl, "POLLDIN", tr, val) == 0)
	generic_set_trigger_threshold(astat, POLLDIN, tr, val);
    if (generic_trigger_item_parse(tl, "POLLDOUT", tr, val) == 0)
	generic_set_trigger_threshold(astat, POLLDOUT, tr, val);
    if (generic_trigger_item_parse(tl, "RRCVPKG", tr, val) == 0)
	generic_set_trigger_threshold(astat, RRCVPKG, tr, val);
    if (generic_trigger_item_parse(tl, "RSNDPKG", tr, val) == 0)
	generic_set_trigger_threshold(astat, RSNDPKG, tr, val);
    if (generic_trigger_item_parse(tl, "DRCVPKG", tr, val) == 0)
	generic_set_trigger_threshold(astat, DRCVPKG, tr, val);
    if (generic_trigger_item_parse(tl, "DSNDPKG", tr, val) == 0)
	generic_set_trigger_threshold(astat, DSNDPKG, tr, val);
    return 0;
}





}
