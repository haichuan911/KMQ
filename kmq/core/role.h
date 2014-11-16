#ifndef _H_ROLE_
#define _H_ROLE_

#include "rbtree.h"
#include "rid.h"
#include "epoller.h"
#include "list.h"
#include "sectionreader.h"
#include "mqp.h"
#include "role_stat_module.h"


KMQ_DECLARATION_START

enum ROLEATTR {
    ATTRR = 0x01,
    ATTRW = 0x02,
    ATTRRW = ATTRR|ATTRW,
};

enum MSGEV {
    MSGIN = 0x01,
    MSGOUT = 0x02,
    MSGERROR = 0x04,
};

struct lb_rrbin_data_t {
    int64_t weight;
    rbtree_node_t weight_node;
};

struct lb_random_data_t {

};

struct lb_iphash_data_t {

};

struct lb_fair_data_t {

};

#define ROLEOFEV(ev) ((Role *)(ev)->ptr)
#define list_r(link) ((Role *)link->self) 
#define list_first_r(head)						\
    ({struct list_link *__pos =						\
	    list_first(head, struct list_link, node);  list_r(__pos);})

class Role;
class rattr_event_change_handler {
 public:
    virtual ~rattr_event_change_handler() {}
    virtual int role_enable_r_event(Role *r) = 0;
    virtual int role_disable_r_event(Role *r) = 0;
    virtual int role_enable_w_event(Role *r) = 0;
    virtual int role_disable_w_event(Role *r) = 0;
};

class Role : public msg_queue_monitor {
 public:
    Role(const string &appid, const string &roleid, int rtype);
    virtual ~Role();
    friend class RoleManager;

    int Bind(Conn *conn);
    int unBind(Conn **conn);
    string Id();
    const char *cid();
    const char *cip();
    const char *cappid();
    uint32_t Type();
    Conn *Connect();
    module_stat *Stat();
    int connect_mode();
    int on_rattr_change(rattr_event_change_handler *h);
    int trigger_attr_change_event(int attr, bool enable);
    uint32_t GetAttr();
    void SetAttr(uint32_t attr);
#define ROLECANR(r) ({bool __can = (r->GetAttr() & ATTRR) ? true : false; __can;})
#define ROLECANW(r) ({bool __can = (r->GetAttr() & ATTRW) ? true : false; __can;})
#define SETROLE_R(r) ({int __attr = (r)->GetAttr(); (r)->SetAttr(__attr | ATTRR); \
	    (r)->trigger_attr_change_event(ATTRR, true); __attr;})
#define SETROLE_W(r) ({int __attr = (r)->GetAttr(); (r)->SetAttr(__attr | ATTRW); \
	    (r)->trigger_attr_change_event(ATTRW, true); __attr;})
#define SETROLE_RW(r) ({int __attr = (r)->GetAttr(); SETROLE_R(r); SETROLE_W(r); __attr;})
#define UNSETROLE_R(r) ({int __attr = (r)->GetAttr(); (r)->SetAttr(__attr & ~ATTRR); \
	    (r)->trigger_attr_change_event(ATTRR, false); __attr;})
#define UNSETROLE_W(r) ({int __attr = (r)->GetAttr(); (r)->SetAttr(__attr & ~ATTRW); \
	    (r)->trigger_attr_change_event(ATTRW, false); __attr;})
#define UNSETROLE_RW(r) ({int __attr = (r)->GetAttr(); UNSETROLE_R(r); UNSETROLE_W(r); __attr;})
    
    uint32_t GetMsgev();
    void SetMsgev(uint32_t ev);
#define HAS_MSGIN(r) ((r)->GetMsgev() & MSGIN)
#define HAS_MSGOUT(r) ((r)->GetMsgev() & MSGOUT)
#define HAS_MSGERROR(r) ((r)->GetMsgev() & MSGERROR)    
#define CLEAR_MSGEV(r) ((r)->SetMsgev(0))
#define SET_MSGIN(r) ((r)->SetMsgev((r)->GetMsgev() | MSGIN))
#define SET_MSGOUT(r) ((r)->SetMsgev((r)->GetMsgev() | MSGOUT))
#define SET_MSGERROR(r) ((r)->SetMsgev((r)->GetMsgev() | MSGERROR))
#define CLEAR_MSGIN(r) ((r)->SetMsgev((r)->GetMsgev() & ~MSGIN))
#define CLEAR_MSGOUT(r) ((r)->SetMsgev((r)->GetMsgev() & ~MSGOUT))
#define CLEAR_MSGERROR(r) ((r)->SetMsgev((r)->GetMsgev() & ~MSGERROR))

    
    int HandleMsgev();
    int AttachToMsgevHead(struct list_head *in, struct list_head *out, struct list_head *err);
    int resource_stat_module_timestamp_update(int64_t timestamp);
    
    int AttachPoller(Epoller *eplr, int to_msec);
    int DetachPoller();
    int EnablePOLLIN();
    int DisablePOLLIN();
    int EnablePOLLOUT();
    int DisablePOLLOUT();

    int attach_to_mqwaiter_head(struct list_head *head);
    int detach_from_mqwaiter_head();
    int mq_empty_callback_func();
    int mq_nonempty_callback_func();

    int attach_to_rolemgr_head(struct list_head *head);
    int detach_from_rolemgr_head();
    int attach_to_msgin_head(struct list_head *head);
    int detach_from_msgin_head();
    int attach_to_msgout_head(struct list_head *head);
    int detach_from_msgout_head();
    int attach_to_msgerror_head(struct list_head *head);
    int detach_from_msgerror_head();
    
    int InitResource(MQueuePool *mqp, Epoller *elpr, int to_msec);
    int ReleaseResource();
    MQueue *Queue();
    
    virtual void Reset() = 0;
    virtual int Recv(struct kmqmsg **msg) = 0;
    virtual int Send(struct kmqmsg *msg) = 0;
    virtual int send_icmp() = 0;
    virtual int BatchSend(int max_send) = 0;

    int PushSNDICMP(struct kmqmsg *icmp_msg);
    struct kmqmsg *PopSNDICMP();
    int PushRCVICMP(struct kmqmsg *icmp_msg);
    struct kmqmsg *PopRCVICMP();
    
 private:
    int inited;
    uint32_t roletype, roleattr;
    rattr_event_change_handler *rattr_change_h;
    module_stat rstat;
    __role_stat_module_trigger __role_sm;

    uint32_t msgev;
    string app, id, raddr;
    Conn *internconn;
    EpollEvent ee;
    Epoller *poller;
    rbtree_node_t dd_timeout_node;
    struct list_link rolemgr_node;
    struct list_link mqwaiter_node;
    struct list_link msgin_node;
    struct list_link msgout_node;
    struct list_link msgerror_node;

    union {
	struct lb_rrbin_data_t rb;
	struct lb_random_data_t rd;
	struct lb_iphash_data_t ip;
	struct lb_fair_data_t fr;
    } loadbalance_data;

    MQueue *mqueue;
    MQueuePool *mqpool;
    msg_queue icmp_snd_queue, icmp_rcv_queue;
};




}



#endif  // _H_ROLE_
