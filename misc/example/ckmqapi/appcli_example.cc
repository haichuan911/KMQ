#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <kmq/compatapi.h>

using namespace kmq;

static string appname = "testapp";
static string apphost = "127.0.0.1:1510";

static int handler_siginit() {
    // Ignore SIGPIPE
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN))
        return -1;
    return 0;
}



int main(int argc, char **argv) {
    CSpioApi client;
    string msg("i am client");
    int ret = 0;

    handler_siginit();
    client.init(appname);
    client.join_client(apphost);

    while (1) {
	sleep(1);
	string resp;
	// Read request msg
	if ((ret = client.send(msg, 5000)) == 0) {
	    client.recv(resp, 5000);
	    fprintf(stdout, "client send: %s\n", msg.c_str());
	} else {
	    fprintf(stdout, "client send error: %s\n", kmq_strerror(errno));
	}
    }
    return 0;
}
