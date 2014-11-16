#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <gtest/gtest.h>
#include "ht.h"
#include "osthread.h"


using namespace kmq;


static int cnt = 100;

class MyRpp : public MpHandler {
public:
    MyRpp() {
	cnt = to_cnt = 0;
    }
    ~MyRpp() {}
    int TimeOut(int to_msec) {
	to_cnt++;
	return 0;
    }
    int ServeKMQ(const char *data, int len) {
	cnt++;
	return 0;
    }
    int cnt, to_cnt;
};

static int callback_worker(void *arg_) {
    HandlerTable *ht = (HandlerTable *)arg_;
    vector<int> hids;
    vector<int>::iterator it;
    MyRpp mr, *cb = NULL;
    int i = 0, hid = 0;

    for (i = 0; i < cnt; i++) {
	if ((hid = ht->GetHid()) < 0)
	    continue;
	hids.push_back(hid);
	EXPECT_EQ(0, ht->InsertHandler(hid, &mr, rand() % 50));
	if (i == cnt/2)
	    usleep(250000);
    }
    for (it = hids.begin(); it != hids.end(); ++it) {
	hid = *it;
	if ((cb = (MyRpp *)ht->FindHandler(hid)))
	    cb->cnt++;
    }
    EXPECT_NE(0, mr.cnt);
    EXPECT_NE(0, mr.to_cnt);
    EXPECT_EQ(cnt, mr.cnt + mr.to_cnt);
    return 0;
}


static int ht_test() {
    int w = 4, i;
    OSThread t[4];
    HandlerTable ht;

    ht.Init(100);
    for (i = 0; i < w; i++)
	t[i].Start(callback_worker, &ht);
    for (i = 0; i < w; i++)
	t[i].Stop();
    return 0;
}


TEST(ht, handlertable) {
    ht_test();
}
