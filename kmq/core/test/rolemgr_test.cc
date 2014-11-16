#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include <string>
#include "net.h"
#include "log.h"
#include "appctx.h"

using namespace std;
using namespace kmq;


static int rolemanager(void *arg_) {
    RoleManager rom;
    Conn *conn;
    Role *r;
    struct kmqreg header = {};
    int cnt = 50, i = 0;

    strcpy(header.appname, "testapp");
    for (i = 0; i < cnt; i++) {
	conn = new TCPConn();
	if (rand() % 3 == 0)
	    header.rtype = ROLE_RECEIVER;
	else
	    header.rtype = ROLE_DISPATCHER;
	uuid_generate(header.rid);
	rom.Register(&header, conn);
    }

    for (i = 0; i < cnt; i++) {
	if (NULL != (r = rom.PopNewer())) {
	    r->unBind(&conn);
	    rom.TimeWait(r);
	    delete conn;
	} else
	    KMQLOG_ERROR("invalid poprole %d", i);
	break;
    }
    return 0;
}


TEST(role, manager) {
    rolemanager(NULL);
}
