#include <gtest/gtest.h>
#include <sys/signal.h>
#include <kmq/test/test.h>
#include "ut.h"
#include "os.h"
#include "log.h"


using namespace kmq;

int gargc = 0;
char **gargv = NULL;

int main(int argc, char **argv) {

    int ret;
    
    gargc = argc;
    gargv = argv;

    // Ignore SIGPIPE
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
        fprintf(stderr, "signal SIG_IGN");
        return -1;
    }
    testing::InitGoogleTest(&gargc, gargv);

    KMQLOG_CONFIG(DOTEST_LOGGER_CONF);

    kmq_os_init();
    ret = RUN_ALL_TESTS();

    KMQLOG_SHUTDOWN();
    return ret;
}
