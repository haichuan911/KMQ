#ifndef _H_PROTOCOL_H_
#define _H_PROTOCOL_H_

#include <stdio.h>
#include <uuid/uuid.h>
#include <string.h>
#include <kmq/kmq.h>
#include "slice.h"
#include "list.h"
#include "CRC.h"

KMQ_DECLARATION_START


/*
enum FLAG_BITS {
// for icmp
    FLAG_BIT1 = 0x0001,
    FLAG_BIT2 = 0x0002,
    FLAG_BIT3 = 0x0004,
    FLAG_BIT4 = 0x0008,

// for async api    
    FLAG_BIT5 = 0x0010,

// unused    
    FLAG_BIT6 = 0x0020,
    FLAG_BIT7 = 0x0040,
    FLAG_BIT8 = 0x0080,
    FLAG_BIT9 = 0x0100,
    FLAG_BIT10 = 0x0200,
    FLAG_BIT11 = 0x0400,
    FLAG_BIT12 = 0x0800,
    FLAG_BIT13 = 0x1000,
    FLAG_BIT14 = 0x2000,
    FLAG_BIT15 = 0x4000,
    FLAG_BIT16 = 0x8000,
};
*/




enum {
    HEADER_BUFFERLEN = 1024
};
 
#define RHDRLEN ((int)sizeof(struct kmqhdr))

struct kmqrt {
    union {
	struct {
	    uuid_t uuid;
	    uint32_t sendstamp;
	} env;
	char __padding[20];
    } u;
};
#define RTLEN ((int)sizeof(struct kmqrt))






#define MAX_APPNAME_LEN 64
enum ROLETYPE {
    ROLE_APPRECEIVER = 0x01,
    ROLE_PIORECEIVER = 0x02,
    ROLE_APPDISPATCHER = 0x04,
    ROLE_PIODISPATCHER = 0x08,
    ROLE_RECEIVER = ROLE_APPRECEIVER|ROLE_PIORECEIVER,
    ROLE_DISPATCHER = ROLE_APPDISPATCHER|ROLE_PIODISPATCHER,
    ROLE_MONITOR = 0x10,
};
#define IS_RECEIVER(_rt) (!!(_rt & ROLE_RECEIVER))
#define IS_PIORECEIVER(_rt) (!!(_rt & ROLE_PIORECEIVER))
#define IS_DISPATCHER(_rt) (!!(_rt & ROLE_DISPATCHER))
#define IS_PIODISPATCHER(_rt) (!!(_rt & ROLE_PIODISPATCHER))
#define ROLESTR(_r) (IS_RECEIVER(_r->Type()) ? "r" : "d")

struct kmqreg {
    uint32_t version;
    uint32_t rtype;
    int64_t timeout;
    uuid_t rid;
    char appname[MAX_APPNAME_LEN];
};

#define REGHDRLEN ((int)sizeof(kmqreg))



static inline int header_parse(struct kmqhdr *header, char *out) {
    return snprintf(out, HEADER_BUFFERLEN, 
		    "{version:%d ttl:%d flag:%x size:%u"
		    " timestamp:%"PRIu64" hdrcheck:%d datacheck:%d}",
		    header->version, header->ttl, header->flags, header->size,
		    header->timestamp, header->hdrcheck, header->datacheck);
}

 
struct appmsg_rt {
    uint8_t ttl;
    char __padding[];
};

static inline uint32_t appmsg_rtlen(int ttl) {
    return sizeof(struct appmsg_rt) + ttl * RTLEN;
}

struct appmsg {
    struct kmqhdr hdr;
    struct slice s;
    struct appmsg_rt *rt;
    struct list_head node;
};

#define list_first_appmsg(head)						\
    ({struct appmsg *__msg = list_first(head, struct appmsg, node); __msg;})
#define list_for_each_appmsg(pos, head)				\
    list_for_each_entry((pos), (head), struct appmsg, node)
#define list_for_each_appmsg_safe(pos, next, head)			\
    list_for_each_entry_safe((pos), (next), (head), struct appmsg, node)

struct kmqmsg {
    struct kmqhdr hdr;
    char *data;
    struct kmqrt *route;
    struct list_head mq_node;
};

#define MSGHDRLEN ((int)sizeof(struct kmqmsg))
#define MSGPKGLEN(msg) (MSGHDRLEN + (msg)->hdr.size)

#define list_first_m(head)				\
    list_first(head, struct kmq_massage, mq_node);

#define list_for_each_massage(pos, head)			\
    list_for_each_entry((pos), struct kmq_massage, mq_node)

#define list_for_each_massage_safe(pos, next, head)			\
    list_for_each_entry_safe((pos), (next), (head), struct kmq_massage, mq_node)


class Conn;
class proto_parser {
 public:
    proto_parser();
    ~proto_parser();

    inline void _set_recv_max(uint32_t max_size) {
	msg_max_size = max_size;
    }
    inline uint64_t _dropped_cnt() {
	return dropped_cnt;
    }
    int _reset_recv();
    int _reset_send();

    int _recv_massage(Conn *conn, struct kmqhdr *hdr, struct slice *s);
    int _send_massage(Conn *conn, const struct kmqhdr *hdr, int c, struct slice *s);
    int _send_massage_async(Conn *conn, const struct kmqhdr *hdr, int c, struct slice *s);

    int _recv_massage(Conn *conn, struct kmqmsg **header, int reserve);
    int _send_massage(Conn *conn, struct kmqmsg *header);
    int _send_massage_async(Conn *conn, struct kmqmsg *header);
#ifndef __KMQ_UT__
 private:
#endif
    uint64_t dropped_cnt;
    uint32_t msg_max_size;
    struct kmqmsg msg, *bufhdr;
    uint32_t recv_hdr_index;
    uint32_t recv_data_index;
    uint32_t send_hdr_index;
    uint32_t send_data_index;

    int __drop(Conn *conn, uint32_t size);
    int __recv_hdr(Conn *conn, struct kmqhdr *hdr);
    int __recv_data(Conn *conn, int c, struct slice *s);
    int __send_hdr(Conn *conn, const struct kmqhdr *hdr);
    int __send_data(Conn *conn, int c, struct slice *s);
};


static inline int package_validate(struct kmqhdr *header, void *data) {
    struct kmqhdr copyheader = *header;

    copyheader.hdrcheck = 0;
    if (crc16((char *)&copyheader, RHDRLEN) != header->hdrcheck)
	return -1;
    if (header->datacheck &&
	data && crc16((char *)data, header->size) != header->datacheck)
	return -1;
    return 0;
}


static inline int package_makechecksum(struct kmqhdr *header, void *data, uint32_t len) {
    struct kmqhdr copyheader = *header;
    copyheader.hdrcheck = 0;
    if (data && len) {
	copyheader.datacheck = header->datacheck = crc16((char *)data, len);
    }
    header->hdrcheck = crc16((char *)&copyheader, RHDRLEN);
    return 0;
}










}








#endif // _H_PROTOCOL_H_
