#ifndef _H_KMQICMP_
#define _H_KMQICMP_

#include "proto.h"

KMQ_DECLARATION_START

enum ICMPFLAG {
    ICMPMSG = FLAG_BIT1,
    ICMP_ROLEATTR = FLAG_BIT2 | ICMPMSG,
    ICMP_APPSTATUS = FLAG_BIT3 | ICMPMSG,
    ICMP_ROLESTATUS = FLAG_BIT4 | ICMPMSG,
};

#define IS_ICMP(msg) (((msg)->hdr.flags & ICMPMSG))

struct role_status_icmp {
    int dispatchers;
};

struct kmqmsg *make_role_status_icmp(int dispatchers);


}
#endif   // _H_KMQICMP_
