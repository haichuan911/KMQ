#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <vector>
#include "os.h"
#include "path.h"
#include "config.h"

using namespace std;
using namespace kmq;

extern string default_kmq_conf_name;
extern SpioConfig kmq_conf;
extern int32_t g_rtime;


class _appsconf_vector {
public:
    vector<string> vec;
};

static void _walk_appsconf(const string &path, void *data) {
    _appsconf_vector *apps_conf = (_appsconf_vector *)data;
    if (HasSuffix(path, ".conf"))
	apps_conf->vec.push_back(path);
}


extern int kmqd_start_apps(vector<string> &apps_conf);
extern int kmqd_stop_apps();
extern int kmqd_start_regmgr(SpioConfig *kmq_conf);
extern int kmqd_stop_regmgr();
extern int kmqd_start_monitor();
extern int kmqd_stop_monitor();
extern int kmqd_global_init(int argc, char **argv);
extern int kmqd_global_exit();

int kmq_main(int argc, char **argv) {
    int64_t start_tt = 0, up_interval = 0;
    FilePath fp;
    _appsconf_vector apps_conf;

    if (kmqd_global_init(argc, argv) < 0 || kmqd_start_regmgr(&kmq_conf) < 0)
	return -1;
    kmqd_start_monitor();

    up_interval = kmq_conf.appconfupdate_interval_sec * 1E3;
    fp.Setup(kmq_conf.apps_configdir);
    while (g_rtime > 0) {
	if (!start_tt || rt_mstime() - start_tt > up_interval) {
	    apps_conf.vec.clear();
	    fp.WalkFile(_walk_appsconf, &apps_conf);
	    if (apps_conf.vec.size())
		kmqd_start_apps(apps_conf.vec);
	    start_tt = rt_mstime();
	}
	sleep(1);
	g_rtime--;
    }

    kmqd_stop_monitor();
    kmqd_stop_apps();
    kmqd_stop_regmgr();

    kmqd_global_exit();
    return 0;
}


int main(int argc, char **argv) {
    return kmq_main(argc, argv);
}
