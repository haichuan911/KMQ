#include <iostream>
#include <stdlib.h>
#include <gtest/gtest.h>


static int test_even_odd_algorithm() {
    int idc_id[10], i, token;

    token = rand();
    
    for (i = 0; i < 10; i++) {
	idc_id[i] = rand();
	token ^= idc_id[i];
    }

    for (i = 10 - 1; i >= 0; i--) {
	
    }
    return 0;
}


TEST(even_odd_check, route) {
    EXPECT_EQ(0, test_even_odd_algorithm());
}
