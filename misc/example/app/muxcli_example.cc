#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kmq/kmqmux.h>

using namespace kmq;


static string appname = "testapp";
static string apphost = "127.0.0.1:1510";    


class MyObj : public MpHandler {
    /* ... */
public:
    int incr_ref() {
	ref++;
    }
    int decr_ref() {

    }
private:
    pmutex_t lock;
    int ref;
};

int MyObj::TimeOut(int to_msec) {
    lock(&lock);
    ref--;
    /* do some thing */
    if (ref == 0)
	delete this;
    unlock(&lock);
}

int MyObj::ServeKMQ(const char *data, int len) {
    lock(&lock);
    ref--;
    if (ref == 0)
	delete this;
    unlock(&lock);
}



int main(int argc, char **argv) {
    kmq_request *rr = NULL;

    MuxComsuer *mux = NewMuxComsuer();
    MuxComsuer *mux1 = NewMuxComsuer();
    MuxComsuer *mux2 = NewMuxComsuer();
    
    /* init ... */

    mux->connect_to_kmq(kmqsvr1, appname);
    mux1->connect_to_kmq(kmqsvr2, appname);
    mux2->connect_to_kmq(kmqsvr3, appname);

    while (1) {
	rr = recv_kmq_massage();
	MyObj *obj = new MyObj();
	obj->init(rr);

	rr.incr_ref();
	rr.incr_ref();
	rr.incr_ref();

	mux.sendrequest(somedata, len, rr, 10);
	mux1.sendrequest(somedata, len, rr, 10);
	mux2.sendrequest(somedata, len, rr, 10);
    }

    
    /* exit ... */
    return 0;
}




