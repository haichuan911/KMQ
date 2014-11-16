#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include "os.h"
#include "ht.h"
#include "memalloc.h"


KMQ_DECLARATION_START



HandlerTable::HandlerTable() :
    inited(0), slots(0), slot_cleanup_idx(0), seqid(0), hts(NULL)
{
    tmutex_init(&lock);
}

HandlerTable::~HandlerTable() {
    int i = 0;
    kmq_respcallback_t *pos = NULL, *next = NULL;
    struct hlist_head *head = NULL;

    if (!inited)
	return;
    tmutex_destroy(&lock);
    for (i = 0; i < slots; i++) {
	tmutex_destroy(&hts[i].lock);
	head = &hts[i].head;
	hlist_for_each_entry_safe(pos, next, head, kmq_respcallback_t, cb_link) {
	    hlist_del(&pos->cb_link);
	    mem_free(pos, sizeof(*pos));
	}
    }
    mem_free(hts, slots * sizeof(*hts));
}


int HandlerTable::Init(int size) {
    int i;
    kmq_respcallback_head_t *ht = NULL;

    if (inited) {
	errno = KMQ_EDUPOP;
	return -1;
    }
    if (size <= 0) {
	errno = EINVAL;
	return -1;
    }
    if ((hts = (kmq_respcallback_head_t *)
	 mem_zalloc(sizeof(*hts) * size)) == NULL) {
	errno = ENOMEM;
	return -1;
    }
    for (i = 0; i < size; i++) {
	ht = &hts[i];
	tmutex_init(&ht->lock);
	INIT_HLIST_HEAD(&ht->head);
	rbtree_init(&ht->timer_rbtree, &ht->timer_sentinel, rbtree_insert_timer_value);
    }
    inited = 1;
    slots = size;
    return 0;
}


int64_t HandlerTable::GetHid() {
    int64_t hid = -1;

    if (!inited) {
	errno = KMQ_EINTERN;
	return -1;
    }
    tmutex_lock(&lock);
    hid = seqid;
    seqid++;
    tmutex_unlock(&lock);
    return hid;
}



int HandlerTable::InsertHandler(int64_t hid, MpHandler *handler, int to_msec) {
    kmq_respcallback_t *rcp = NULL;
    kmq_respcallback_head_t *ht = NULL;

    if (!inited) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (!(rcp = (kmq_respcallback_t *)mem_zalloc(sizeof(*rcp)))) {
	errno = ENOMEM;
	return -1;
    }
    rcp->hid = hid;
    rcp->callback = handler;
    INIT_HLIST_NODE(&rcp->cb_link);
    ht = &hts[hid % slots];
    tmutex_lock(&ht->lock);
    hlist_add_head(&rcp->cb_link, &ht->head);
    rcp->ctime = rt_mstime();
    if (to_msec) {
	rcp->rblinked = 1;
	rcp->timer_node.key = rcp->ctime + to_msec;
	rcp->timer_node.data = rcp;
	rbtree_insert(&ht->timer_rbtree, &rcp->timer_node);
    }
    tmutex_unlock(&ht->lock);
    return 0;
}


int HandlerTable::__cleanup_tw_callbacks(kmq_respcallback_head_t *ht) {
    int64_t cur_mstime = 0;
    rbtree_node_t *node = NULL;
    kmq_respcallback_t *rcp = NULL;

    cur_mstime = rt_mstime();
    while (ht->timer_rbtree.root != ht->timer_rbtree.sentinel) {
	node = rbtree_min(ht->timer_rbtree.root, ht->timer_rbtree.sentinel);
	if (node->key > cur_mstime)
	    break;
	rcp = (kmq_respcallback_t *)node->data;
	hlist_del(&rcp->cb_link);
	rbtree_delete(&ht->timer_rbtree, &rcp->timer_node);
	rcp->callback->TimeOut(cur_mstime - rcp->ctime);
	mem_free(rcp, sizeof(*rcp));
    }
    return 0;
}


int HandlerTable::cleanup_tw_callbacks() {
    int i = 0, tc = 1;
    kmq_respcallback_head_t *ht = NULL;

    if (!inited) {
	errno = KMQ_EINTERN;
	return -1;
    }
    if (tc <= 0)
	tc = 1;
    for (i = 0; i < tc; i++) {
	if (slot_cleanup_idx >= slots)
	    slot_cleanup_idx = 0;
	ht = &hts[slot_cleanup_idx];
	tmutex_lock(&ht->lock);
	__cleanup_tw_callbacks(ht);
	tmutex_unlock(&ht->lock);
	slot_cleanup_idx++;
    }
    return 0;
}



MpHandler *HandlerTable::FindHandler(int64_t hid) {
    MpHandler *rh = NULL;
    kmq_respcallback_head_t *ht = NULL;
    kmq_respcallback_t *pos = NULL, *next = NULL;

    ht = &hts[hid % slots];
    tmutex_lock(&ht->lock);
    __cleanup_tw_callbacks(ht);
    hlist_for_each_entry_safe(pos, next, &ht->head, kmq_respcallback_t, cb_link) {
	if (pos->hid != hid)
	    continue;
	hlist_del(&pos->cb_link);
	if (pos->rblinked)
	    rbtree_delete(&ht->timer_rbtree, &pos->timer_node);
	rh = pos->callback;
	mem_free(pos, sizeof(*pos));
	break;
    }
    tmutex_unlock(&ht->lock);
    return rh;
}












}
