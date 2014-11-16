#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include "rbtree.h"

using namespace std;
using namespace kmq;


static int rbtree_test_single(int argc, char **argv) {
    int i, cnt = 10000;
    std::vector<int64_t > allval, allval2;
    int64_t min_val = 0;
    rbtree_t tree;
    rbtree_node_t sentinel, *node, *min_node;

    rbtree_init(&tree, &sentinel, rbtree_insert_value);
    for (i = 0; i < cnt; i++) {
	node = (rbtree_node_t *)malloc(sizeof(rbtree_node_t));
	if (!node) {
	    printf("alloc failed\n");
	    break;
	}
	node->key = rand() + 1;
	if (min_val == 0 || node->key < min_val)
	    min_val = node->key;
	allval.push_back(node->key);
	rbtree_insert(&tree, node);
	min_node = rbtree_min(tree.root, &sentinel);
	if (min_node->key != min_val) {
	    printf("rbtree internal error. invalid min_val: %"PRIu64" %"PRIu64, min_node->key, min_val);
	    return -1;
	}
    }

    std::sort(allval.begin(), allval.end());
    for (std::vector<int64_t >::iterator it = allval.begin(); it != allval.end(); ++it) {
	min_node = rbtree_min(tree.root, &sentinel);
	if (min_node->key != *it) {
	    printf("rbtree internal error. invalid min_val: %"PRIu64" %"PRIu64, min_node->key, *it);
	    return -1;
	}
	rbtree_delete(&tree, min_node);
	free(min_node);
    }
    return 0;
}


static int rbtree_test_time_value(int argc, char **argv) {
    rbtree_t tree;
    rbtree_node_t sentinel;

    rbtree_init(&tree, &sentinel, rbtree_insert_timer_value);

    return 0;
}


TEST(rbtree, single) {
    EXPECT_EQ(0, rbtree_test_single(1, NULL));
}


TEST(rbtree, time_value) {
    EXPECT_EQ(0, rbtree_test_time_value(1, NULL));
}
