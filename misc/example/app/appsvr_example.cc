#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kmq/kmq.h>


using namespace kmq;

static string appname = "testapp";
static string apphost = "127.0.0.1:1520";


int main(int argc, char **argv) {
    SpioComsuer *server = NULL;
    string reqmsg, rt;
    int ret = 0;

    if ((server = NewSpioComsuer()) == NULL)
	return -1;
    while (server->Connect(appname, apphost) < 0)
	usleep(10);
	
    while (1) {
	sleep(1);
	// Read request msg
	if ((ret = server->RecvMsg(reqmsg, rt)) < 0) {
	    // When error. close and reconnect!
	    server->Close();

	    // kmq server maybe down ...	    
	    // reconnect until kmq server startup
	    while (server->Connect(appname, apphost) < 0)
		usleep(100);
	    continue;
	}
	fprintf(stdout, "server recv: %s\n", reqmsg.c_str());
	// Send response msg
	if ((ret = server->SendMsg(reqmsg, rt)) < 0) {
	    // When error. close and reconnect!
	    server->Close();

	    // kmq server maybe down ...	    	    
	    // reconnect until kmq server startup
	    while (server->Connect(appname, apphost) < 0)
		usleep(100);
	    continue;
	}
    }
    return 0;
}
