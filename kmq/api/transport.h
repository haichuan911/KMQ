#ifndef _H_KMQTRANSPORT_
#define _H_KMQTRANSPORT_

#include <list>
#include "epoller.h"
#include "osthread.h"

using namespace std;

KMQ_DECLARATION_START

typedef int (*transport_worker)(void *data);

class async_transport {
 public:
    async_transport();
    ~async_transport();
    int attach(transport_worker *worker, void *data);
    int start();
    int stop();

 private:
    OSThread iothread;
    list<OSThread *> callback_threads;
    Epoller *poller;
};









}
#endif  //  _H_KMQTRANSPORT_
