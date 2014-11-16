#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include "os.h"


using namespace kmq;

static int clock_test() {
    int64_t start_tt = 0, end_tt = 0;

    start_tt = clock_mstime();
    usleep(2000);
    end_tt = clock_mstime();
    EXPECT_TRUE(end_tt > start_tt);

    start_tt = clock_ustime();
    usleep(2000);
    end_tt = clock_ustime();
    EXPECT_TRUE(end_tt - start_tt > 2000);
    return 0;
}

static int rt_test() {
    int64_t start_tt = 0, end_tt = 0;

    start_tt = rt_mstime();
    usleep(2000);
    end_tt = rt_mstime();
    EXPECT_TRUE(end_tt > start_tt);

    start_tt = rt_ustime();
    usleep(2000);
    end_tt = rt_ustime();
    EXPECT_TRUE(end_tt - start_tt > 2000);
    return 0;
}


TEST(time, rt) {
    rt_test();
}


TEST(time, clock) {
    clock_test();
}
