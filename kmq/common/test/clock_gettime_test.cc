#include <gtest/gtest.h>
#include <time.h>
#include <unistd.h>
#include "osthread.h"
#include "net.h"
#include "epoller.h"

using namespace kmq;

static volatile int tcplisten_status = 0;
static volatile int canstop;

static int tcp_server(void *arg_) {
    TCPListener *listen;
    TCPConn *conn;

    listen = ListenTCP("tcp", "*:20004", 100);
    listen->SetReuseAddr(true);
    EXPECT_TRUE(listen != NULL);
    tcplisten_status = 1;
    conn = listen->Accept();
    while (!canstop) {
	usleep(10);
    }
    delete listen;
    delete conn;
    return 0;
}




static void clock_gettime_test() {
    OSThread svr;
    TCPConn *conn;

    // setting here
    long cnt = 1;
    Epoller *poller;
    EpollEvent ev;
    struct list_head io_head, to_head;

    svr.Start(tcp_server, NULL);
    while (!tcplisten_status) {
	/* waiting ... */
    }

    poller = EpollCreate(500, 10);
    ASSERT_TRUE(poller != NULL);
    conn = DialTCP("tcp", "", "127.0.0.1:20004");
    ASSERT_TRUE(conn != NULL);
    ev.fd = conn->Fd();
    ev.events = EPOLLIN;
    poller->CtlAdd(&ev);
    while (cnt--) {
	INIT_LIST_HEAD(&io_head);
	INIT_LIST_HEAD(&to_head);
	if (poller->Wait(&io_head, &to_head, 100) == 0 && !list_empty(&io_head))
	    ev.detach();
	detach_for_each_poll_link(&io_head);
	detach_for_each_poll_link(&to_head);
    }

    canstop = 1;
    svr.Stop();
    delete conn;
    delete poller;
    return;
}





TEST(time, clock_gettime) {
    clock_gettime_test();
}
