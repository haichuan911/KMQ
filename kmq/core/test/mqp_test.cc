#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include "receiver.h"
#include "mqp.h"


using namespace kmq;

static int msgqueueex_normal(void *arg_) {
    MQueuePool mqp;
    MQueue *mq;
    struct kmqmsg *hdr;
    int cnt = 1000, i, idx;
    string rid;
    vector<string> uuid;

    // first. create queue
    struct mqp_conf mqpconf = {
	1024000, 0, 0
    };
    mqp.Setup("mockapp", &mqpconf);
    for (i = 0; i < cnt; i++) {
	route_genid(rid);
	uuid.push_back(rid);
	mqp.Set(rid);
    }


    // second. random push data from queue
    for (i = 0; i < cnt; i++) {
	idx = rand() % cnt;
	mq = mqp.Find(uuid.at(idx));
	if (mq) {
	    hdr = (struct kmqmsg *)malloc(MSGHDRLEN);
	    memset(hdr, 0, MSGHDRLEN);
	    mq->PushMsg(hdr);
	}
    }

    // third. random pop data from queue
    for (i = 0; i < cnt; i++) {
	idx = rand() % cnt;
	mq = mqp.Find(uuid.at(idx));
	if (mq) {
	    if ((hdr = mq->PopMsg()) != NULL)
		free(hdr);
	}
    }
    return 0;
}


static int msgqueueex_timeout() {
    MQueuePool mqp, mqp2;
    MQueue *mq;
    struct kmqmsg *hdr;
    string rid;

    // first. create queue
    struct mqp_conf mockconf = {
	1024000, 0, 0
    };
    mqp.Setup("mockapp", &mockconf);

    struct mqp_conf mockconf2 = {
	1024000, 0, 10
    };
    mqp2.Setup("mockapp", &mockconf2);

    route_genid(rid);
    mqp.Set(rid);
    mqp2.Set(rid);
    
    // second. random push data from queue
    mq = mqp.Find(rid);

    hdr = (struct kmqmsg *)malloc(MSGHDRLEN);
    memset(hdr, 0, MSGHDRLEN);
    mq->PushMsg(hdr);
    usleep(500);
    hdr = mq->PopMsg();
    EXPECT_TRUE(hdr != NULL);


    mq = mqp2.Find(rid);
    mq->PushMsg(hdr);
    usleep(100000);
    // package is timeout and dropped by queue
    hdr = mq->PopMsg();
    EXPECT_TRUE(hdr == NULL);

    mqp.unSet(rid);
    return 0;
}



TEST(msgqueueex, normal) {
    msgqueueex_normal(NULL);
}


TEST(msgqueueex, timeout) {
    msgqueueex_timeout();
}
