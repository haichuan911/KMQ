#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include <iostream>
#include "log.h"
#include "role_stat_module.h"

using namespace std;

KMQ_DECLARATION_START

const char *role_stat_module_item[ROLE_MODULE_STATITEM_KEYRANGE] = {
    "",
    "transfertime",
    "reconnect",
    "recvbytes",
    "sendbytes",
    "recvpackages",
    "sendpackages",
    "recverrors",
    "senderrors",
    "checksumerrors",
};

__role_stat_module_trigger::__role_stat_module_trigger() :
    _s_trigger_cnt(0), _m_trigger_cnt(0), _h_trigger_cnt(0), _d_trigger_cnt(0)
{

}


__role_stat_module_trigger::~__role_stat_module_trigger() {

}

int __role_stat_module_trigger::setup(const string &appid, const string &id, const string &ip) {
    _appid = appid;
    _rid = id;
    _rip = ip;
    return 0;
}

#define __check_key_range(__key)					\
    do {								\
	if (__key <= 0 || __key > ROLE_MODULE_STATITEM_KEYRANGE) {	\
	    KMQLOG_ERROR("module_stat invalid key %d", __key);	\
	    errno = EINVAL;						\
	    return -1;							\
	}								\
    } while (0)

int __role_stat_module_trigger::trigger_s_threshold(module_stat *self,
						    int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat s_trigger appctx:" << _appid << " role:" << _rid << " ip:" << _rip;
    ss << " [" << role_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}

int __role_stat_module_trigger::trigger_m_threshold(module_stat *self,
						    int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat m_trigger appctx:" << _appid << " role:" << _rid << " ip:" << _rip;
    ss << " [" << role_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}


int __role_stat_module_trigger::trigger_h_threshold(module_stat *self,
						    int key, int64_t threshold, int64_t val) {
    stringstream ss;

    __check_key_range(key);
    ss << "module_stat h_trigger appctx:" << _appid << " role:" << _rid << " ip:" << _rip;
    ss << " [" << role_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}

int __role_stat_module_trigger::trigger_d_threshold(module_stat *self,
						    int key, int64_t threshold, int64_t val) {
    stringstream ss;
    __check_key_range(key);
    ss << "module_stat d_trigger appctx:" << _appid << " role:" << _rid << " ip:" << _rip;
    ss << " [" << role_stat_module_item[key] << ":" << val << "]";
    KMQLOG_NOTICE("%s", ss.str().c_str());
    return 0;
}


int init_role_stat_module_trigger(module_stat *rstat, const string &tl) {
    int tr = 0, val = 0;
    rstat->batch_unset_threshold();
    if (generic_trigger_item_parse(tl, "TRANSFERTIME", tr, val) == 0)
	generic_set_trigger_threshold(rstat, TRANSFERTIME, tr, val);
    if (generic_trigger_item_parse(tl, "RECONNECT", tr, val) == 0)
	generic_set_trigger_threshold(rstat, RECONNECT, tr, val);
    if (generic_trigger_item_parse(tl, "RECV_BYTES", tr, val) == 0)
	generic_set_trigger_threshold(rstat, RECV_BYTES, tr, val);
    if (generic_trigger_item_parse(tl, "SEND_BYTES", tr, val) == 0)
	generic_set_trigger_threshold(rstat, SEND_BYTES, tr, val);
    if (generic_trigger_item_parse(tl, "RECV_PACKAGES", tr, val) == 0)
	generic_set_trigger_threshold(rstat, RECV_PACKAGES, tr, val);
    if (generic_trigger_item_parse(tl, "SEND_PACKAGES", tr, val) == 0)
	generic_set_trigger_threshold(rstat, SEND_PACKAGES, tr, val);
    if (generic_trigger_item_parse(tl, "RECV_ERRORS", tr, val) == 0)
	generic_set_trigger_threshold(rstat, RECV_ERRORS, tr, val);
    if (generic_trigger_item_parse(tl, "SEND_ERRORS", tr, val) == 0)
	generic_set_trigger_threshold(rstat, SEND_ERRORS, tr, val);
    if (generic_trigger_item_parse(tl, "CHECKSUM_ERRORS", tr, val) == 0)
	generic_set_trigger_threshold(rstat, CHECKSUM_ERRORS, tr, val);
    return 0;
}






}
