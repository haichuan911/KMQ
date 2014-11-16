#ifndef _H_KMQCOMPAT_
#define _H_KMQCOMPAT_

#include <iostream>
#include <kmq/kmq.h>


using namespace std;

namespace kmq {

#define MODE_RR 1
#define MODE_ROUTE 2

class CSpioApi {
 public:
    CSpioApi();
    ~CSpioApi();

    int init(const string &groupname);
    int join_client(const string &grouphost);
    int join_server(const string &grouphost);
    int rejoin();
    int recv(string &msg, int timeout = 0);
    int send(const string &msg, int timeout = 0);
    void terminate();

#if defined(__KMQ_UT__)
    int fd();
#endif

 private:
    bool m_joined;
    bool m_server;
    int m_msgid;
    int m_to_msec, m_to_cnt;
    string m_groupname, m_grouphost;
    string cur_rt;
    SpioComsumer *internserver;
    SpioProducer *internclient;
};

}
















#endif   // _H_KMQCOMPAT_
