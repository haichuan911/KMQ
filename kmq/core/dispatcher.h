#ifndef _H_DISPATCHER_H_
#define _H_DISPATCHER_H_

#include "role.h"
#include "proto.h"
#include "list.h"

using namespace std;
KMQ_DECLARATION_START

class MQueue;
class Dispatcher : public Role {
 public:
    Dispatcher(const string &_appid, const string &roleid);
    Dispatcher(int rt, const string &_appid, const string &roleid);
    ~Dispatcher();

    void Reset() {
	pp._reset_recv();
	pp._reset_send();
    }
    int Recv(struct kmqmsg **resp);
    int Send(struct kmqmsg *header);
    int BatchSend(int max_send);
    int send_icmp();
    
 private:
    proto_parser pp;
    
    int process_error();
    int process_normal_massage(struct kmqmsg *msg);

    int process_self_icmp(struct kmqmsg *icmp_msg);
    int process_role_status_icmp(struct kmqmsg *msg);
};















}


#endif // _H_DISPATCHER_H_
