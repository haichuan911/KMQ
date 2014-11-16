#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include "log.h"
#include "lb_iphash.h"
#include "mem_status.h"


KMQ_DECLARATION_START

extern kmq_mem_status_t kmq_mem_stats;

static mem_stat_t *lb_iphash_mem_stats = &kmq_mem_stats.lb_iphash;


lb_iphash::lb_iphash() :
    numbers(0), cap(1), backend_servers(NULL)
{
    do {
	if (!(backend_servers = (Role **)mem_zalloc(cap * sizeof(Role *))))
	    KMQLOG_ERROR("create lb_iphash with errno %d", errno);
    } while (!backend_servers);
    lb_iphash_mem_stats->alloc++;
    lb_iphash_mem_stats->alloc_size += sizeof(lb_iphash);
}

lb_iphash::~lb_iphash() {
    if (backend_servers)
	mem_free(backend_servers, cap * sizeof(Role *));
    lb_iphash_mem_stats->alloc--;
    lb_iphash_mem_stats->alloc_size -= sizeof(lb_iphash);
}


int lb_iphash::size() {
    return numbers;
}

int lb_iphash::balance() {
    return 0;
}

int lb_iphash::add(Role *r) {
    Role **__new_backend_servers = NULL;

    if ((numbers == cap) &&
	(__new_backend_servers =
	 (Role **)mem_realloc(backend_servers, cap * 2 * sizeof(Role *)))) {
	backend_servers = __new_backend_servers;
	cap = cap * 2;
    }
    if (numbers >= cap) {
	KMQLOG_ERROR("realloc lb_iphash with errno %d", errno);
	return -1;
    }
    KMQLOG_INFO("lb_iphash add %s", r->cid());
    backend_servers[numbers] = r;
    numbers++;
    return 0;
}

int lb_iphash::del(Role *r) {
    int i = 0;

    for (i = 0; i < numbers; i++) {
	if (backend_servers[i] != r)
	    continue;
	backend_servers[i] = backend_servers[numbers - 1];
	numbers--;
	KMQLOG_ERROR("lb_iphash del %s", r->cid());
	return 0;
    }
    KMQLOG_ERROR("lb_iphash del %s not found", r->cid());
    return -1;
}


Role *lb_iphash::loadbalance_recv() {
    Role *r = NULL;
    
    return r;
}


Role *lb_iphash::loadbalance_send(struct kmqmsg *msg) {
    Role *r = NULL;
    uint32_t i = 0, hash = 0;
    struct kmqrt *rt = NULL;

    if (!msg) {
	errno = EINVAL;
	goto out;
    }
    if (!backend_servers || numbers <= 0) {
	errno = KMQ_EINTERN;
	goto out;
    }
    rt = msg->route;
    for (i = 0; i < sizeof(rt->u.env.uuid); i++) {  
	hash = (hash * 113 + rt->u.env.uuid[i]) % 6271;
    }
    if (backend_servers && numbers > 0)
	r = backend_servers[hash % numbers];
 out:
    return r;
}


}
