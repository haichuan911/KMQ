#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kmq/errno.h>
#include "os.h"
#include "log.h"
#include "CRC.h"
#include "osthread.h"
#include "epoller.h"
#include "api.h"
#include "benchmark.h"



using namespace kmq;


struct my_msg {
    int64_t msg_timestamp;
    int64_t msg_checksum;
};


class MyApp {
public:
    MyApp() {}
    ~MyApp() {}

    int sendcnt;
    int recvcnt;
    __SpioInternProducer client;
    EpollEvent ee;
};


int kmq_sync_server(void *arg_) {

    Epoller *poller = NULL;
    EpollEvent ee;
    struct list_head io_head, to_head;
    
    __SpioInternComsumer server;
    struct appmsg req = {};
    struct my_msg *_my_msg = NULL;
    int ret = 0, cur_recv = 0;
    int avg_rtt = 0;
    
    INIT_LIST_HEAD(&io_head);
    INIT_LIST_HEAD(&to_head);

    if (!(poller = EpollCreate(1024, 500))) {
	KMQLOG_ERROR("epoll create %s\n", kmq_strerror(errno));
	return -1;
    }
    if ((ret = server.Connect(appname, kmqsvrhost)) < 0) {
	delete poller;
	return -1;
    }
    ee.SetEvent(server.Fd(), EPOLLIN|EPOLLRDHUP, &server);
    poller->CtlAdd(&ee);
    server.SetOption(OPT_NONBLOCK, 1);

    while (!stopping) {
	ee.happened = 0;
	if ((ret = poller->Wait(&io_head, &to_head, 1)) < 0) {
	    printf("epoll wait %s\n", kmq_strerror(errno));
	    break;
	}
	if (ee.happened & EPOLLRDHUP) {
	    printf("kmq benchmark appserver EPOLLRDHUP\n");
	    poller->CtlDel(&ee);
	    server.Close();
	    while (server.Connect(appname, kmqsvrhost) < 0)
		usleep(100000);
	    ee.SetEvent(server.Fd(), EPOLLIN|EPOLLRDHUP, &server);
	    poller->CtlAdd(&ee);
	}
	if (ee.happened & EPOLLIN) {
	    if ((ret = server.RecvMsg(&req)) == 0) {
		_my_msg = (struct my_msg *)req.s.data;
		avg_rtt += rt_mstime() - _my_msg->msg_timestamp;
		cur_recv++;
		if (cur_recv % 10000 == 0 && cur_recv > 0) {
		    // printf("server avg rtt: %d\n", avg_rtt / cur_recv);
		    avg_rtt = 0;
		    cur_recv = 0;
		}
		server.SendMsg(&req);
		free(req.s.data);
		free(req.rt);
	    } else if (ret < 0 && errno != EAGAIN) {
		poller->CtlDel(&ee);
		server.Close();
		while (server.Connect(appname, kmqsvrhost) < 0)
		    usleep(100000);
		ee.SetEvent(server.Fd(), EPOLLIN|EPOLLRDHUP, &server);
		poller->CtlAdd(&ee);
	    }
	}
	detach_for_each_poll_link(&io_head);
	detach_for_each_poll_link(&to_head);
    }
    delete poller;
    return 0;
}






int kmq_sync_client(void *arg_) {

    __SpioInternProducer *client = NULL;
    MyApp *app = NULL;
    Epoller *poller = NULL;
    EpollEvent *ev = NULL;
    struct list_head io_head, to_head;
    
    vector<MyApp *> apps;
    vector<MyApp *>::iterator it;
    
    string req, resp;
    Msghdr hdr = {};
    int i, ret = 0;

    int cur_send = 0, cur_recv = 0, lost = 0;
    int rtt = 0, tmp_rtt = 0;
    int64_t start_tt = 0, end_tt = 0;
    
    INIT_LIST_HEAD(&io_head);
    INIT_LIST_HEAD(&to_head);

    if (!(poller = EpollCreate(1024, 500))) {
	KMQLOG_ERROR("epoll create %s\n", kmq_strerror(errno));
	return -1;
    }

    for (i = 0; i < g_clients; i++) {
	app = new MyApp();
	apps.push_back(app);
	client = &app->client;
	while (client->Connect(appname, kmqclihost) < 0)
	    usleep(10);
	client->SetOption(OPT_NONBLOCK, 1);
    }
    end_tt = rt_mstime() + g_time;
    start_tt = rt_mstime();
    rtt = 0;
    cur_send = cur_recv = 0;

    for (it = apps.begin(); it != apps.end(); ++it) {
	app = *it;
	ev = &app->ee;
	client = &app->client;
	ev->SetEvent(client->Fd(), EPOLLIN|EPOLLRDHUP, app);	    
	poller->CtlAdd(ev);
	app->sendcnt = app->recvcnt = 0;

	req.clear();
	req.assign(buffer, rand() % g_size);
	hdr.timestamp = rt_mstime();
	if (g_check == "yes")
	    hdr.checksum = crc16(req.data(), req.size());
	client->SendMsg(&hdr, req);
	app->sendcnt++;
	cur_send++;
    }
    
    while (rt_mstime() < end_tt) {
	
	if ((poller->Wait(&io_head, &to_head, 1)) < 0 || list_empty(&io_head)) {
	    detach_for_each_poll_link(&to_head);
	    continue;
	}

	list_for_each_poll_link_autodetach(ev, &io_head) {
	    app = (MyApp *)ev->ptr;
	    client = &app->client;

	    if (ev->happened & EPOLLRDHUP) {
		printf("client found kmq EPOLLRDHUP\n");
		poller->CtlDel(ev);
		client->Close();
		while (client->Connect(appname, kmqclihost) < 0)
		    rt_usleep(10000);
		client->SetOption(OPT_NONBLOCK, 1);
		continue;
	    }
	    if (ev->happened & EPOLLIN) {
		resp.clear();
		if ((ret = client->RecvMsg(&hdr, resp)) == 0) {
		    tmp_rtt = rt_mstime() - hdr.timestamp;
		    if (g_check == "yes" && hdr.checksum != crc16(resp.data(), resp.size()))
			printf("client recv error checksum response of line:%d\n", __LINE__);
		    rtt += tmp_rtt;
		    cur_recv++;
		    app->recvcnt++;

		    hdr.timestamp = rt_mstime();		    
		    client->SendMsg(&hdr, resp);
		    app->sendcnt++;
		    cur_send++;
		    resp.clear();
		} else if (ret < 0 && errno != EAGAIN) {
		    poller->CtlDel(ev);
		    continue;
		}
	    }
	}}}

	detach_for_each_poll_link(&io_head);
	detach_for_each_poll_link(&to_head);
    }

    // waiting rest package
    end_tt = rt_mstime() + 1000;

    while (rt_mstime() < end_tt) {
	if ((poller->Wait(&io_head, &to_head, 1)) < 0 || list_empty(&io_head)) {
	    detach_for_each_poll_link(&to_head);
	    continue;
	}

	list_for_each_poll_link_autodetach(ev, &io_head) {
	    app = (MyApp *)ev->ptr;
	    client = &app->client;

	    if (ev->happened & EPOLLRDHUP) {
		poller->CtlDel(ev);
		continue;
	    }
	    if (ev->happened & EPOLLIN) {
		resp.clear();
		if ((ret = client->RecvMsg(&hdr, resp)) == 0) {
		    tmp_rtt = rt_mstime() - hdr.timestamp;
		    if (g_check == "yes" && hdr.checksum != crc16(resp.data(), resp.size())) {
			printf("client recv error checksum response of line:%d\n", __LINE__);
		    }
		    rtt += tmp_rtt;
		    cur_recv++;
		    app->recvcnt++;
		    resp.clear();
		} else if (ret < 0 && errno != EAGAIN) {
		    poller->CtlDel(ev);
		    continue;
		}
	    }
	}}}

	detach_for_each_poll_link(&io_head);
	detach_for_each_poll_link(&to_head);
    }

    if (cur_recv) {
	if ((lost = cur_send - cur_recv) < 0)
	    lost = 0;
	printf("client send: %10d  recv: %10d  avg_rtt: %2d lost: %.6f  tp: %"PRId64"\n",
	       cur_send, cur_recv, rtt / cur_recv, (float)lost / (float)cur_send,
	       cur_recv * 1000 / (end_tt - start_tt));
    }
    for (it = apps.begin(); it != apps.end(); ++it) {
	app = *it;
	ev = &app->ee;
	poller->CtlDel(ev);
    }

    for (it = apps.begin(); it != apps.end(); ++it)
	delete *it;
    delete poller;

    return 0;
}

