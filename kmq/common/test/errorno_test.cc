#include <gtest/gtest.h>
#include <kmq/errno.h>
#include "osthread.h"

using namespace kmq;


static int AThread(void *arg_) {
    errno = KMQ_EROUTE;
    return 0;
}


static int test_errorno() {
    OSThread thread;

    errno = 0;
    thread.Start(AThread, NULL);
    thread.Stop();
    EXPECT_TRUE(errno != KMQ_EROUTE);
    errno = KMQ_EROUTE;
    EXPECT_TRUE(errno == KMQ_EROUTE);
    return 0;
}


static int test_kmq_errno() {
    const char *errptr = NULL;

    EXPECT_TRUE((errptr = kmq_strerror(KMQ_ERRNO - 1)) != NULL);
    EXPECT_TRUE((errptr = kmq_strerror(KMQ_ERRNO)) != NULL);    
    EXPECT_TRUE((errptr = kmq_strerror(KMQ_ECONFIGURE)) != NULL);
    EXPECT_TRUE((errptr = kmq_strerror(KMQ_ETIMEOUT)) != NULL);
    EXPECT_TRUE((errptr = kmq_strerror(KMQ_EKMQDOWN)) != NULL);

    return 0;
}


TEST(errorno, threadsafe) {
    test_errorno();
}

TEST(errorno, kmqerrno) {
    test_kmq_errno();
}
