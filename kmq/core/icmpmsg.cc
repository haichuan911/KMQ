#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "icmpmsg.h"
#include "memalloc.h"

KMQ_DECLARATION_START



struct kmqmsg *make_role_status_icmp(int dispatchers) {
    struct kmqmsg *msg = NULL;
    struct role_status_icmp status = {};
    int role_status_icmplen = sizeof(status);

    if (!(msg = (struct kmqmsg *)mem_zalloc(MSGHDRLEN + role_status_icmplen)))
	return NULL;
    msg->hdr.size = role_status_icmplen;
    msg->hdr.flags = ICMP_ROLESTATUS;
    msg->data = (char *)msg + MSGHDRLEN;
    status.dispatchers = dispatchers;
    memcpy(msg->data, &status, role_status_icmplen);
    return msg;
}




}
