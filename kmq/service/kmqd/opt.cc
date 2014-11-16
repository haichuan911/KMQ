#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <signal.h>
#include "os.h"
#include "log.h"
#include "config.h"
#include "daemon.h"
#include "pid_ctrl.h"

using namespace std;
using namespace kmq;

SpioConfig kmq_conf;
string default_kmq_conf_name = "kmq.conf";
string g_kmqconf;
string g_cmd;
string g_daemon = "yes";
int32_t g_rtime = 0x7fffffff;

static char _usage_data[] = "\n\
NAME\n\
    kmq - proxy io\n\
\n\
SYNOPSIS\n\
    kmq -c configdir -s start|stop|restart [-r runtime] [-d no]\n\
\n\
OPTIONS\n\
    -c kmq configure dir. see example: %{_prefix}/share/kmq/conf\n\
    -s service cmd. start|stop|restart\n\
    -r service running time. default = 68years\n\
    -d daemon. default daemon = yes\n\n\
\n\
EXAMPLE:\n\
    kmq -c /home/w/share/kmq/conf -s start -d no\n\n";





static void kmq_usage(const char* exeName) {
    system("clear");
    printf("%s", _usage_data);
}

static int kmq_getoption (int argc, char* argv[]) {
    int rc;

    while ( (rc = getopt(argc, argv, "c:s:r:d:h")) != -1 ) {
        switch(rc) {
        case 'c':
	    g_kmqconf = optarg;
	    g_kmqconf += "/" + default_kmq_conf_name;
            break;
        case 's':
            if(strcmp(optarg,"start") == 0 || strcmp(optarg,"restart") == 0 ||
               strcmp(optarg,"stop") == 0) {
                g_cmd = optarg;
            } else {
                kmq_usage(argv[0]);
                return -1;
            }
            break;
	case 'r':
	    g_rtime = atoi(optarg);
	    if (g_rtime == 0)
		g_rtime = 0x7fffffff;
	    break;
	case 'd':
	    g_daemon = optarg;
	    break;
        case 'h':
	    kmq_usage(argv[0]);
	    return -1;
        }
    }
    if (g_kmqconf == "" || g_cmd == "") {
	kmq_usage(argv[0]);
	exit(-1);
    }
    return 0;
}

static int kmq_ignore_signal() {
    // ignore SIGPIPE signal
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
	fprintf(stderr, "signal SIGPIPE ignore\n");
	return -1;
    }
    return 0;
}

static int kmq_process_command() {
    pid_ctrl_t pc("kmq", "daemon", NULL);
    pid_ctrl_t::EPidCtrlResult reason;

    if (g_cmd == "stop" || g_cmd == "restart") {
	if (!pc.kill(SIGTERM)) {
	    fprintf(stderr, "permission denied when stop or restart\n");
	}
	if (g_cmd == "stop")
	    return -1;
    }
    if (pc.exists(reason)) {
	switch (reason) {
	case pid_ctrl_t::PCR_PERMISSION_DENIED:
	    fprintf(stderr, "permission denied\n");
	    break;
	case pid_ctrl_t::PCR_RUNNING_ALREADY:
	    fprintf(stderr, "running already\n");
	    break;
	case pid_ctrl_t::TOTAL_PCR:
	default:
	    break;
	}
	return -1;
    }
    if (g_daemon == "yes")
	init_daemon();
    sleep(1);
    pc.savePid();
    return 0;
}


int kmqd_global_init(int argc, char **argv) {
    kmq_os_init();
    kmq_ignore_signal();

    if (kmq_getoption(argc, argv) < 0 || kmq_process_command() < 0)
	return -1;
    if ((kmq_conf.Init(g_kmqconf.c_str())) < 0) {
	fprintf(stderr, "kmq init: %s\n", g_kmqconf.c_str());
	return -1;
    }
    if (!kmq_conf.log4conf.empty())
	KMQLOG_CONFIG(kmq_conf.log4conf);
    return 0;
}


int kmqd_global_exit() {
    if (!kmq_conf.log4conf.empty())
	KMQLOG_SHUTDOWN();
    return 0;
}
