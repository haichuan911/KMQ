#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include <string.h>
#include "memalloc.h"
#include "mux.h"



KMQ_DECLARATION_START


SpioReadWriter::SpioReadWriter() :
    req(NULL), mux_comsumer(NULL)
{

}

SpioReadWriter::~SpioReadWriter() {

}


const char *SpioReadWriter::Data() {
    return req->s.data;
}

uint32_t SpioReadWriter::Len() {
    return req->s.len;
}

string SpioReadWriter::Route() {
    string rt;
    rt.append((char *)&req->hdr, sizeof(req->hdr));
    rt.append((char *)req->rt, appmsg_rtlen(req->rt->ttl));
    return rt;
}

int SpioReadWriter::Send(const char *data, uint32_t len, const string &rt) {
    return mux_comsumer->SendResponse(data, len, rt);
}

}
