#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "regmgr/regmgr.h"


using namespace std;
using namespace kmq;

Rgm *rgm = NULL;

int kmqd_start_regmgr(SpioConfig *kmq_conf) {
    set<string>::iterator it;

    if (!(rgm = NewRegisterManager(kmq_conf))) {
	KMQLOG_ERROR("start regmgr failed of oom");
	return -1;
    }
    for (it = kmq_conf->regmgr_listen_addrs.begin();
	 it != kmq_conf->regmgr_listen_addrs.end(); ++it) {
	rgm->Listen(*it);
    }
    return rgm->StartThread();
}


void kmqd_stop_regmgr() {
    if (rgm) {
	rgm->StopThread();
	delete rgm;
    }
}

