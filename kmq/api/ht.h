#ifndef _H_HQ_
#define _H_HQ_

#include <kmq/kmqmux.h>
#include "tmutex.h"
#include "list.h"
#include "rbtree.h"


KMQ_DECLARATION_START

typedef struct resp_callback_head {
    tmutex_t lock;
    struct hlist_head head;
    rbtree_t timer_rbtree;
    rbtree_node_t timer_sentinel;
} kmq_respcallback_head_t;

typedef struct resp_callback {
    int64_t hid, ctime;
    MpHandler *callback;
    int rblinked;
    rbtree_node_t timer_node;
    struct hlist_node cb_link;
} kmq_respcallback_t;
 


class HandlerTable {
 public:
    HandlerTable();
    ~HandlerTable();

    int Init(int size);
    int64_t GetHid();
    int cleanup_tw_callbacks();
    int InsertHandler(int64_t hid, MpHandler *handler, int to_msec = 0);
    MpHandler *FindHandler(int64_t hid);

 private:
    int inited;
    int slots, slot_cleanup_idx;
    int64_t seqid;
    tmutex_t lock;
    kmq_respcallback_head_t *hts;

    int __cleanup_tw_callbacks(kmq_respcallback_head_t *head);
};











}



#endif   // _H_HQ_
