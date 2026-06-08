#include "server-sm.h"
#include <signal.h>
int main(void) {
    signal(SIGPIPE, SIG_IGN);
    int rv = handle_srv_states();

    return rv;
}
