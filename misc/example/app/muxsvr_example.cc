#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kmq/kmqmux.h>

using namespace kmq;


static string appname = "testapp";
static string apphost = "127.0.0.1:1510";    

class EchoServer : public MuxHandler {
public:
    int ServeMux(ReqReader &rr, ResponseWriter &rw);
};

int EchoServer::ServeMux(ReqReader &rr, ResponseWriter &rw) {
    string msg;
    rr.Recv(msg);
    rw.Send(msg);
    return 0;
}



int main(int argc, char **argv) {
    EchoServer es;
    MuxComsuer *mux = NewMuxComsuer();

    mux->Setup(appname, apphost, 20);
    mux->Handle(&es);

    // backend thread start...
    mux->StartServe(); 

    /* do anything by yourself here*/ 
    while (1) {

    }

    // stop mux
    mux->Stop();
    return 0;
}
