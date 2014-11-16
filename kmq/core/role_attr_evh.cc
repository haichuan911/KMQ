#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "role_attr_evh.h"

KMQ_DECLARATION_START

rattr_ev_monitor::rattr_ev_monitor() :
    canr_receivers(0), canw_receivers(0), canr_dispatchers(0), canw_dispatchers(0)
{

}

rattr_ev_monitor::~rattr_ev_monitor() {

}

int rattr_ev_monitor::role_enable_r_event(Role *r) {
    if (IS_RECEIVER(r->Type()))
	canr_receivers++;
    else if (IS_DISPATCHER(r->Type()))
	canr_dispatchers++;
    return 0;
}

int rattr_ev_monitor::role_disable_r_event(Role *r) {
    if (IS_RECEIVER(r->Type()))
	canr_receivers--;
    else if (IS_DISPATCHER(r->Type()))
	canr_dispatchers--;
    return 0;
}

int rattr_ev_monitor::role_enable_w_event(Role *r) {
    if (IS_RECEIVER(r->Type()))
	canw_receivers++;
    else if (IS_DISPATCHER(r->Type()))
	canw_dispatchers++;
    return 0;
}

int rattr_ev_monitor::role_disable_w_event(Role *r) {
    if (IS_RECEIVER(r->Type()))
	canw_receivers--;
    else if (IS_DISPATCHER(r->Type()))
	canw_dispatchers--;
    return 0;
}



}
