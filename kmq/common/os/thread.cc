#include <unistd.h>
#include <kmq/errno.h>
#include "os.h"
#include "osthread.h"


KMQ_DECLARATION_START

static void *thread_routine (void *arg_)
{
    OSThread *self = (OSThread*) arg_;

    self->tid = gettid();
    self->tret = self->tfn (self->m_arg);

    return NULL;
}

int OSThread::Start (OSThread_fn *tfn_, void *arg_)
{
    tfn = tfn_;
    m_arg =arg_;
    int rc = pthread_create (&m_descriptor, NULL, thread_routine, this);
    return rc;
}

int OSThread::Stop ()
{
    void *status;

    pthread_join (m_descriptor, &status);
    return tret;
}


}
