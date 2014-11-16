#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/epoll.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <kmq/errno.h>
#include "os.h"
#include "epoller.h"

KMQ_DECLARATION_START

EpollEvent::EpollEvent() :
    udtype(0), ptr(NULL), fd(-1), to_nsec(0), events(0), happened(0)
{
    memset(&timer_node, 0, sizeof(timer_node));
    timer_node.data = this;
    INIT_LIST_LINK(&link);
}

int EpollEvent::attach(struct list_head *head) {
    if (!link.linked) {
	link.linked = 1;
	list_add(&link.node, head);
    }
    return 0;
}
    
int EpollEvent::detach() {
    if (link.linked) {
	list_del(&link.node);
	link.linked = 0;
    }
    return 0;
}
    
EpollEvent::~EpollEvent() {

}


void EpollEvent::SetEvent(int _fd, uint32_t _events, void *data) {
    fd = _fd;
    events = _events;
    ptr = data;
}




Epoller::Epoller(int efd, int size, int _max_io_evs) :
    epoll_fd(efd), max_io_events(_max_io_evs),
    evcnt(0), iocnt(0), tocnt(0)
{
    rbtree_init(&timer_rbtree, &timer_sentinel, rbtree_insert_timer_value);
}

Epoller::~Epoller() {
    if (epoll_fd >= 0)
	close(epoll_fd);
}


int Epoller::Ctl(int op, EpollEvent *ev) {
    switch (op) {
    case EPOLL_CTL_ADD:
	return CtlAdd(ev);
    case EPOLL_CTL_MOD:
	return CtlMod(ev);
    case EPOLL_CTL_DEL:
	return CtlDel(ev);
    }
    return -1;
}

int Epoller::CtlAdd(EpollEvent *ev) {
    int ret = 0;
    int64_t _cur_nsec = rt_nstime();
    epoll_event ee;
    
    if (ev->to_nsec) {
	ev->timer_node.key = _cur_nsec + ev->to_nsec;
	rbtree_insert(&timer_rbtree, &ev->timer_node);
    }

    if (ev->fd >= 0) {
	ee.events = ev->events;
	ee.data.ptr = ev;
	if ((ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev->fd, &ee)) != -1) {
	    iocnt++;
	    evcnt++;
	}
    } else {
	/* a pure timeout event */
	ret = 0;
	evcnt++;
	tocnt++;
    }
    return ret;
}

int Epoller::CtlMod(EpollEvent *ev) {
    int ret = 0;
    int64_t _cur_nsec = rt_nstime();
    epoll_event ee;

    if (ev->to_nsec) {
	if (ev->timer_node.key)
	    rbtree_delete(&timer_rbtree, &ev->timer_node);
	ev->timer_node.key = _cur_nsec + ev->to_nsec;
	rbtree_insert(&timer_rbtree, &ev->timer_node);
    }

    if (ev->fd >= 0) {
	ee.events = ev->events;
	ee.data.ptr = ev;
	ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev->fd, &ee);
    } else { /* a pure timeout event */ }

    return ret;
}

int Epoller::CtlDel(EpollEvent *ev) {
    int ret = 0;
    epoll_event ee;

    if (ev->to_nsec && ev->timer_node.key) {
	rbtree_delete(&timer_rbtree, &ev->timer_node);
	ev->timer_node.key = 0;
    }

    if (ev->fd >= 0) {
	ee.events = ev->events;
	ee.data.ptr = ev;
	if ((ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev->fd, &ee)) != -1) {
	    iocnt--;
	    evcnt--;
	}
    } else {
	/* a pure timeout event */
	ret = 0;
	evcnt--;
	tocnt--;
    }

    return ret;
}


static int64_t find_timer(rbtree_node_t *root, rbtree_node_t *sentinel, int64_t to) {
    rbtree_node_t *node = NULL;
    if (root == sentinel)
	return to;
    node = rbtree_min(root, sentinel);
    if (node->key < to)
	return node->key;
    return to;
}


int Epoller::Wait(struct list_head *io_head, struct list_head *to_head, int msec) {
    EpollEvent *ev = NULL;
    epoll_event ev_buf[max_io_events];
    int ret = 0, i = 0, cnt = 0;
    rbtree_node_t *node = NULL;
    int64_t timeout = 0, to = 0, _cur_nsec = rt_nstime();

    if (!io_head || !to_head) {
	errno = EINVAL;
	return -1;
    }
    to = _cur_nsec + msec * 1000000;
    timeout = find_timer(timer_rbtree.root, &timer_sentinel, to);
    timeout = timeout > _cur_nsec ? timeout - _cur_nsec : 0;
    if (iocnt) {
	if ((ret = epoll_wait(epoll_fd, ev_buf,
			      max_io_events, timeout/1000000)) > 0 && ret <= iocnt)
	    cnt = ret;
    } else
	usleep(timeout/1000);

    for (i = 0; i < cnt; i++) {
	ev = (EpollEvent *)ev_buf[i].data.ptr;
	if (ev->timer_node.key && ev->timer_node.key < _cur_nsec) {
	    rbtree_delete(&timer_rbtree, &ev->timer_node);
	    ev->timer_node.key = 0;
	    ev->happened = EPOLLTIMEOUT;
	    ev->attach(to_head);
	} else {
	    ev->happened = ev_buf[i].events;
	    ev->attach(io_head);
	}
    }

    cnt = max_io_events - cnt;
    while (cnt && timer_rbtree.root != timer_rbtree.sentinel) {
	node = rbtree_min(timer_rbtree.root, timer_rbtree.sentinel);
	if (node->key > _cur_nsec)
	    break;
	ev = (EpollEvent *)node->data;
	rbtree_delete(&timer_rbtree, &ev->timer_node);
	ev->timer_node.key = 0;
	ev->happened = EPOLLTIMEOUT;
	ev->attach(to_head);
	cnt--;
    }

    return 0;
}


Epoller *EpollCreate(int size, int max_io_events) {
    int efd = 0;
    Epoller *ep = NULL;

    if (-1 == (efd = epoll_create(size)))
	return NULL;
    if (!(ep = new (std::nothrow) Epoller(efd, size, max_io_events)))
	close(efd);
    return ep;
}


}
