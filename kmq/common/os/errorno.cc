#include <kmq/errno.h>
#include <string.h>
#include "decr.h"

KMQ_DECLARATION_START

static char kmq_errmsg[][ESTRLEN] = {
    /* 201 */ "incorrect kmq config",
    /* 202 */ "incorrect route infomation",
    /* 203 */ "incorrect kmq package",
    /* 204 */ "timeout",
    /* 205 */ "registry failed",
    /* 206 */ "network unreachable",
    /* 207 */ "bad connection",
    /* 208 */ "duplicate operation",
    /* 209 */ "kmq server down",
    /* 210 */ "internal state error",
};


char *kmq_strerror(int _errno) {
    char errmsg[ESTRLEN] = {};

    if (_errno > KMQ_ERRNO && _errno < KMQ_ERRNOEND)
	return kmq_errmsg[_errno - KMQ_ERRNO - 1];
    if (_errno <= KMQ_ERRNO || _errno >= KMQ_ERRNOEND)
	return strerror_r(_errno, errmsg, ESTRLEN);
    return NULL;
}


}
