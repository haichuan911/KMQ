#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "load_balance.h"
#include "lb_rrbin.h"
#include "lb_iphash.h"
#include "lb_random.h"
#include "lb_fair.h"

KMQ_DECLARATION_START


LBAdapter *MakeLBAdapter(int lbalgo, ...) {
    LBAdapter *lbp = NULL;

    switch (lbalgo) {
    case LB_RRBIN:
	lbp = new (std::nothrow) lb_rrbin();
	return lbp;
    case LB_RANDOM:
	lbp = new (std::nothrow) lb_random();
	return lbp;
    case LB_IPHASH:
	lbp = new (std::nothrow) lb_iphash();
	return lbp;
    case LB_FAIR:
	lbp = new (std::nothrow) lb_fair();
	return lbp;
    }
    errno = EINVAL;
    return NULL;
}

















}
