#ifndef _H_ROUTEID_
#define _H_ROUTEID_

#include <iostream>
#include <uuid/uuid.h>
#include "proto.h"

using namespace std;

KMQ_DECLARATION_START


#define UUID_LEN ((int)(sizeof(uuid_t)))
#define UUID_STRLEN ((int)(sizeof(uuid_t) * 2 + 1 + 4))


static inline void route_genid(string &rid) {
    uuid_t uu;
    char __uu[UUID_STRLEN] = {};
    uuid_generate(uu);
    uuid_unparse(uu, __uu);
    rid.assign(__uu);
}

static inline string route_cid(const string &rid) {
    char out[UUID_STRLEN] = {};
    uuid_unparse((const unsigned char *)rid.data(), out);
    return out;
}

static inline void route_parse(const struct kmqrt *route, string &rid) {
    string __uu;
    __uu.assign((const char *)route->u.env.uuid, sizeof(uuid_t));
    rid = route_cid(__uu);
}

static inline void route_unparse(const string &rid, struct kmqrt *route) {
    uuid_parse(rid.data(), route->u.env.uuid);
}

static inline void rgh_parse(const struct kmqreg *rgh, string &rid) {
    string __uu;
    __uu.assign((const char *)rgh->rid, sizeof(uuid_t));
    rid = route_cid(__uu);
}

static inline void rgh_unparse(const string &rid, struct kmqreg *rgh) {
    uuid_parse(rid.data(), rgh->rid);
}


}


#endif   // _H_ROUTEID_
