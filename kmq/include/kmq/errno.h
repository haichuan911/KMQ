#ifndef _ERRORNO_INCLUDE_H_
#define _ERRORNO_INCLUDE_H_


#include <errno.h>

namespace kmq {

#define ESTRLEN 128
    
enum kmq_errno {
    KMQ_ERRNO                                = 200,
    KMQ_ECONFIGURE                           = 201,
    KMQ_EROUTE                               = 202,
    KMQ_EPACKAGE                             = 203,
    KMQ_ETIMEOUT                             = 204,
    KMQ_EREGISTRY                            = 205,
    KMQ_ENETUNREACH                          = 206,
    KMQ_EBADCONN                             = 207,
    KMQ_EDUPOP                               = 208,
    KMQ_EKMQDOWN                            = 209,
    KMQ_EINTERN                              = 210,
    KMQ_ERRNOEND,
};

char *kmq_strerror(int _errno);

}

#endif //_ERRORNO_INCLUDKMQ_EH_
