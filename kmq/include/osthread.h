#ifndef _H_OSTHREAD_
#define _H_OSTHREAD_


#include <pthread.h>
#include "decr.h"


KMQ_DECLARATION_START

typedef int (OSThread_fn) (void*);
class OSThread {
 public:
    OSThread () : tfn(NULL), m_arg(NULL), tret(0),
	tid(0) 
    {
    }

    int Start (OSThread_fn *tfn_, void *arg_);
    int Stop ();

    OSThread_fn *tfn;
    void * m_arg;
    int tret;
    pid_t tid;
 private:
    pthread_t m_descriptor;
    OSThread (const OSThread&);
    const OSThread &operator = (const OSThread&);
};


}


#endif  // _H_OSTHREAD_
