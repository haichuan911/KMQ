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


int kmq_except_server(void *arg_) {
    Epoller *poller = NULL;
    EpollEvent ee;
    struct list_head io_head, to_head;
    __SpioInternComsumer server;
    string req, rt;
    int fd;
    int i = 0, ret = 0, pkgs = 1000;

    INIT_LIST_HEAD(&io_head);
    INIT_LIST_HEAD(&to_head);

    if (!(poller = EpollCreate(1024, 500))) {
	KMQLOG_ERROR("epoll create: %s\n", kmq_strerror(errno));
	return -1;
    }
    if ((ret = server.Connect(appname, kmqsvrhost)) < 0) {
	delete poller;
	printf("server connect to %s: %s\n", kmqsvrhost.c_str(), kmq_strerror(errno));
	return -1;
    }
    fd = server.Fd();
    ee.SetEvent(fd, EPOLLIN|EPOLLRDHUP, &server);
    poller->CtlAdd(&ee);

    while (1) {
	for (i = 0; i < pkgs; i++) {
	    // waiting events happen
	    while (1) {
		if (poller->Wait(&io_head, &to_head, 5) < 0) {
		    continue;
		}
		if (!list_empty(&io_head) && (0 == server.RecvMsg(req, rt))) {
		    detach_for_each_poll_link(&io_head);
		    detach_for_each_poll_link(&to_head);
		    break;
		}
		if ((!list_empty(&io_head) && (ee.happened & (EPOLLERR|EPOLLRDHUP))) ||
		    (rand() % 239 == 0)) {
		    poller->CtlDel(&ee);
		    server.Close();
		    while (server.Connect(appname, kmqsvrhost) < 0)
			usleep(100000);
		    ee.SetEvent(server.Fd(), EPOLLIN|EPOLLRDHUP, &server);
		    poller->CtlAdd(&ee);
		    printf("server reconnect ok\n");
		    detach_for_each_poll_link(&io_head);
		    detach_for_each_poll_link(&to_head);
		    continue;
		}
	    }
	    if (rand() % 239 == 0)
		write(fd, buffer, rand() % buflen);
	    if (req.size()) {
		server.SendMsg(req, rt);
	    }
	    req.clear();
	    rt.clear();
	}
	printf("server recv %d pkgs\n", pkgs);
    }

    delete poller;
    return 0;
}



int kmq_except_client(void *arg_) {
    Epoller *poller = NULL;
    EpollEvent ee;
    struct list_head io_head, to_head;
    __SpioInternProducer client;

    int ret = 0, i, fd = 0;
    Msghdr hdr = {};
    string req, resp;
    int64_t tt, stt, ett, cur_ms, cur_ms2;
    int mypkgs = 0, allpkgs = 0;
    int speed = 0, waitcnt = 0, lost = 0, ttcnt = 0;
    

    INIT_LIST_HEAD(&io_head);
    INIT_LIST_HEAD(&to_head);

    if (!(poller = EpollCreate(1024, 500))) {
	KMQLOG_ERROR("epoll create %s\n", kmq_strerror(errno));
	return -1;
    }
    if ((ret = client.Connect(appname, kmqclihost)) < 0) {
	delete poller;
	printf("client connect to %s: %s\n", kmqclihost.c_str(), kmq_strerror(errno));
	return -1;
    }
    client.SetOption(OPT_NONBLOCK, 1);
    cur_ms = stt = rt_mstime();
    ett = stt + g_time * 1000;

    fd = client.Fd();
    ee.SetEvent(fd, EPOLLIN|EPOLLRDHUP, &client);
    poller->CtlAdd(&ee);

    client.RecvMsg(&hdr, resp);
    while (1) {
	mypkgs = rand() % 100;
	for (i = 0; i < mypkgs; i++) {
	    req.clear();
	    req.assign(buffer, rand() % g_size);
	    hdr.timestamp = rt_mstime();
	    if (g_check == "yes")
		hdr.checksum = crc16(req.data(), req.size());
	    client.SendMsg(&hdr, req);
	}

	for (i = 0, ttcnt = 0; i < mypkgs; i++) {
	    // waiting for events happend
	    waitcnt = 1;
	    while (waitcnt) {
		ee.happened = 0;
		if (poller->Wait(&io_head, &to_head, 10) < 0)
		    continue;
		if (!list_empty(&io_head))
		    ee.detach();
		if ((ee.happened & (EPOLLERR|EPOLLRDHUP)) ||
			   (ret < 0 && errno != EAGAIN) ||
			   rand() % 234 == 0) {
		    poller->CtlDel(&ee);
		    client.Close();
		    while (client.Connect(appname, kmqclihost) < 0)
			usleep(100000);
		    cur_ms = rt_mstime();
		    ee.SetEvent(client.Fd(), EPOLLIN|EPOLLRDHUP, &client);
		    poller->CtlAdd(&ee);
		    printf("client reconnect, send %d req and recv %d resp\n", mypkgs, i);
		    goto out;
		}
		if (ee.happened & EPOLLIN) {
		    ret = client.RecvMsg(&hdr, resp);
		    break;
		}
		waitcnt--;
	    }
	    if (resp.size()) {
		ttcnt++;
		if (g_check == "yes" && hdr.checksum != crc16(resp.data(), resp.size())) {
		    printf("collapse invalid response\n");
		}
		resp.clear();
	    }
	    if (rand() % 239 == 0)
		write(fd, buffer, rand() % buflen);
	}
    out:
	allpkgs += mypkgs;
	lost += mypkgs - ttcnt;
	cur_ms2 = rt_mstime();
	if (cur_ms2 > ett)
	    break;
	if (cur_ms2 - cur_ms > 1000) {
	    tt = (cur_ms2 - cur_ms)/allpkgs;
	    speed = 1000 * allpkgs / (cur_ms2 - cur_ms);
	    printf("client send %10d lost %10d. avg tt: %"PRIu64"ms speed: %10dp/s\n",
		   allpkgs, lost, tt, speed);
	    allpkgs = 0;
	    lost = 0;
	    cur_ms = cur_ms2;
	}
    }

    delete poller;
    return 0;
}



