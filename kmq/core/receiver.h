#ifndef _H_RECEIVER_H_
#define _H_RECEIVER_H_

#include <iostream>
#include "os.h"
#include "role.h"
#include "proto.h"
#include "list.h"


using namespace std;


KMQ_DECLARATION_START

class Receiver : public Role {
 public:
    Receiver(const string &_appid, const string &roleid);
    Receiver(int rt, const string &_appid, const string &roleid);
    ~Receiver();

    void Reset() {
	pp._reset_send();
	pp._reset_recv();
    }
    int Recv(struct kmqmsg **req);
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







#endif  // _H_RECEIVER_H_
