#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <kmq/kmq.h>

using namespace std;
using namespace kmq;


static string appname = "testapp";
static string apphost = "127.0.0.1:1510";

int main(int argc, char **argv) {
    SpioProducer *client = NULL;
    string reqmsg("i am kmq client"), respmsg;
    int ret = 0;

    if ((client = NewSpioProducer()) == NULL)
	return -1;
    while (client->Connect(appname, apphost) < 0)
	usleep(10);

    while (1) {
	sleep(1);
	if ((ret = client->SendMsg(reqmsg)) < 0) {
	    // When error . close and reconnect!
	    client->Close();

	    // kmq server maybe down ...
	    // reconnect until kmq server startup
	    while (client->Connect(appname, apphost) < 0)
		usleep(100); 
	    continue;
	}
	fprintf(stdout, "client send: %s\n", reqmsg.c_str());
	if ((ret = client->RecvMsg(respmsg)) < 0) {
	    // When error . close and reconnect!
	    client->Close();

	    // kmq server maybe down ...
	    // reconnect until kmq server startup
	    while (client->Connect(appname, apphost) < 0)
		usleep(100);
	    continue;
	}
    }
    return 0;
}
