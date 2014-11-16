#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "appctx.h"
#include "memalloc.h"
#include "icmpmsg.h"


KMQ_DECLARATION_START

static int __send_role_status_icmp(Role *r, void *data) {
    int *dispatchers = (int *)data;
    struct kmqmsg *icmp_msg = NULL;

    if (r->Type() != ROLE_PIORECEIVER)
	return -1;
    if (!(icmp_msg = make_role_status_icmp(*dispatchers))) {
	KMQLOG_ERROR("%s make role status icmp with errno %d", r->cid(), errno);
	return -1;
    }
    if (r->PushSNDICMP(icmp_msg) < 0) {
	KMQLOG_ERROR("%s push role status icmp with errno %d", r->cid(), errno);
	mem_free(icmp_msg, MSGPKGLEN(icmp_msg));	
	return -1;
    }
    r->send_icmp();
    KMQLOG_INFO("%s push role status icmp", r->cid());
    return 0;
}


int AppCtx::broadcast_role_status_icmp() {
    int dispatchers = ratm.count_canw_dispatchers();
    rom.WalkReceivers(__send_role_status_icmp, &dispatchers);
    return 0;
}



}
