#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "mem_status.h"
#include "appctx.h"


KMQ_DECLARATION_START


int AppCtx::process_console_show(int fd) {
    FILE *fp = NULL;
    pid_t pid = gettid();

    if (fd < 0) {
	fp = stdout;
    } else {
	if (!(fp = fdopen(fd, "w"))) {
	    KMQLOG_ERROR("fdopen of %s", kmq_strerror(errno));
	    return -1;
	}
    }
    fprintf(fp, "\nApp status: %s PID(%d)\n", cid(), pid);
    fprintf(fp, "%10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
	    "name", "pollrin", "pollrout", "polldin", "polldout",
	    "rrcvpkg", "rsndpkg", "drcvpkg", "dsndpkg");

#define __output_app_status(op, name)					\
    fprintf(fp, "%10s %10"PRId64" %10"PRId64" %10"PRId64" %10"PRId64	\
	    " %10"PRId64" %10"PRId64" %10"PRId64" %10"PRId64"\n", name,	\
	    astat.getkey_s(POLLRIN, op), astat.getkey_s(POLLROUT, op),	\
	    astat.getkey_s(POLLDIN, op), astat.getkey_s(POLLDOUT, op),	\
	    astat.getkey_s(RRCVPKG, op), astat.getkey_s(RSNDPKG, op),	\
	    astat.getkey_s(DRCVPKG, op), astat.getkey_s(DSNDPKG, op));

    __output_app_status(MIN, "min");
    __output_app_status(MAX, "max");
    __output_app_status(AVG, "avg");
    __output_app_status(NOW, "now");
#undef __output_app_status
    
    rom.OutputRoleStatus(fp);
    fprintf(fp, "\n\n\nMQueue status:\n");
    mqp.OutputQueueStatus(fp);

    return 0;
}




}
