#define __STDC_FORMAT_MACROS
#define __KMQ_UT__
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kmq/errno.h>
#include "os.h"
#include "log.h"
#include "CRC.h"
#include "osthread.h"
#include "epoller.h"
#include "api.h"


using namespace kmq;


static int randstr(char *buf, int len) {
    int i, idx;
    char token[] = "qwertyuioplkjhgfdsazxcvbnm1234567890";
    for (i = 0; i < len; i++) {
	idx = rand() % strlen(token);
	buf[i] = token[idx];
    }
    return 0;
}

enum {
    SYNC_MODE = 1,
    ASYNC_MODE = 2,
    EXP_MODE = 3,
    COMPAT_MODE = 4,
    TREND_MODE = 5,
};

#define buflen 10240
char buffer[buflen] = {};
string g_kmqconf = "/home/w/conf/kmq/benchmark/log4cpp.conf";
string g_check = "no";
string g_roles = "cs";
int g_mode = SYNC_MODE;
int g_threads = 1;
int g_clients = 1;
int g_servers = 1;
int g_size = 512;
int g_pkgs = 10;
int g_freq = 200;
int g_time = 10000;
volatile int stopping = 0;
string appname = "testapp";
string kmqclihost = "127.0.0.1:1510";
string kmqsvrhost = "127.0.0.1:1520";

typedef int (*kmq_app_client_func)(void *arg_);
typedef int (*kmq_app_server_func)(void *arg_);

extern int kmq_async_server(void *arg_);
extern int kmq_async_client(void *arg_);
extern int kmq_sync_server(void *arg_);
extern int kmq_sync_client(void *arg_);
extern int kmq_except_server(void *arg_);
extern int kmq_except_client(void *arg_);
extern int kmq_compat_server(void *arg_);
extern int kmq_compat_client(void *arg_);
extern int kmq_trend_server(void *arg_);
extern int kmq_trend_client(void *arg_);

static char _usage_data[] = "\n\
NAME\n\
    kmq_benchmark - kmq benchmark tools\n\
\n\
SYNOPSIS\n\
    kmq_benchmark [-t time] [-s size] [-w threads]\n\
                    [-i clients] [-m mode] [-c kmqserver]\n\
                    [-g yes] [-r s|c|cs]\n\
\n\
OPTIONS\n\
    -a appname testapp default.\n\
    -t test time(s). 10s default.\n\
    -s msg size(bytes). default = 512bytes\n\
    -w the number of worker threads. each worker will start N clients.\n\
    -i the number of msg input clients. 1 default.\n\
    -m async or sync mode. sync default.\n\
    -c kmq server ip address. 127.0.0.1 default\n\
    -g checksum for msg data, no default\n\
    -f async freqs\n\
    -r only startup this role. default cs.\n\
        c: client\n\
        s: server\n\
        cs: client and server\n\
\n						\
\n\
EXAMPLE:\n\
    kmq_benchmark -w 8 -s 512 -c '10.83.8.22' -t 5\n\n";



static void usage(const char* exeName)
{
    system("clear");
    printf("%s", _usage_data);
}

static int get_option(int argc, char* argv[])
{
    int rc, _freq = 0, _workers = 0, _size = 0, _time = 0;
    string _mode;

    while ( (rc = getopt(argc, argv, "a:c:f:s:w:i:o:t:m:g:r:p:h")) != -1 ) {
        switch(rc) {
	case 'a':
	    appname = optarg;
	    if (appname.empty())
		appname = "testapp";
	    break;
        case 'c':
            kmqclihost = optarg;
	    kmqsvrhost = optarg;
	    kmqclihost += ":1510";
	    kmqsvrhost += ":1520";
            break;
	case 'f':
	    if ((_freq = atoi(optarg)) != 0)
		g_freq = _freq;
	    break;
	case 's':
	    if ((_size = atoi(optarg)) != 0 && _size < buflen)
		g_size = _size;
	    break;
        case 'w':
	    if ((_workers = atoi(optarg)) != 0)
		g_threads = _workers;
	    break;
        case 'i':
	    if ((_workers = atoi(optarg)) != 0)
		g_clients = _workers;
	    break;
        case 'o':
	    if ((_workers = atoi(optarg)) != 0)
		g_servers = _workers;
	    break;
        case 't':
	    if ((_time = atoi(optarg)) != 0)
		g_time = _time * 1000;
	    break;
        case 'm':
	    _mode = optarg;
	    if (_mode == "sync")
		g_mode = SYNC_MODE;
	    else if (_mode == "async")
		g_mode = ASYNC_MODE;
	    else if (_mode == "exp")
		g_mode = EXP_MODE;
	    else if (_mode == "comp")
		g_mode = COMPAT_MODE;
	    else if (_mode == "trend")
		g_mode = TREND_MODE;
	    break;
        case 'g':
	    if ((g_check = optarg) != "yes")
		g_check = "no";
	    break;
        case 'r':
	    g_roles = optarg;
	    break;
        case 'p':
	    int _pkgs;
	    if ((_pkgs = atoi(optarg)) != 0)
		g_pkgs = _pkgs;
	    break;
	case 'h':
        default:
            usage(argv[0]);
            return -1;
        }
    }
    return 0;
}


int main(int argc, char **argv) {
    vector<OSThread *> app_servers, app_clients;
    vector<OSThread *>::iterator it;
    OSThread *thread = NULL;
    int i = 0;
    kmq_app_client_func cfunc = kmq_sync_client;
    kmq_app_server_func sfunc = kmq_sync_server;
    
    kmq_os_init();

    // Ignore SIGPIPE
    randstr(buffer, buflen);
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN))
        return -1;
    if (get_option(argc, argv) < 0)
	return -1;

    if (0 != access(g_kmqconf.c_str(), F_OK))
	g_kmqconf = "conf/benchmark/log4cpp.conf";
    if (!g_kmqconf.empty())
	KMQLOG_CONFIG(g_kmqconf);

    // Start app_servers and app_clients
    switch (g_mode) {
    case SYNC_MODE:
	sfunc = kmq_sync_server;
	cfunc = kmq_sync_client;
	break;
    case ASYNC_MODE:
	sfunc = kmq_async_server;
	cfunc = kmq_async_client;
	break;
    case EXP_MODE:
	sfunc = kmq_except_server;
	cfunc = kmq_except_client;
	break;
    case COMPAT_MODE:
	cfunc = kmq_compat_client;
	sfunc = kmq_compat_server;
	break;
    case TREND_MODE:
	cfunc = kmq_trend_client;
	sfunc = kmq_trend_server;
	break;
    }

    if (g_roles == "s" || g_roles == "cs") {
	for (i = 0; i < g_threads; i++) {
	    thread = new OSThread();
	    app_servers.push_back(thread);
	    thread->Start(sfunc, NULL);
	}
    }
    if (g_roles == "c" || g_roles == "cs") {
	for (i = 0; i < g_threads; i++) {
	    thread = new OSThread();
	    app_clients.push_back(thread);
	    thread->Start(cfunc, NULL);
	}
    }

    if (g_roles == "c" || g_roles == "cs") {
	for (it = app_clients.begin(); it != app_clients.end(); ++it) {
	    thread = *it;
	    thread->Stop();
	    delete thread;
	}
	stopping = 1;
    }

    // Wait appservers and appclients stop
    if (g_roles == "s" || g_roles == "cs") {
	for (it = app_servers.begin(); it != app_servers.end(); ++it) {
	    thread = *it;
	    thread->Stop();
	    delete thread;
	}
    }
    
    if (!g_kmqconf.empty())
	KMQLOG_SHUTDOWN();
    return 0;
}
