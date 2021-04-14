#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "portctrl.h"

static char *ports;
static unsigned int ports_max;
static unsigned int ports_base;
static pthread_mutex_t ports_mutex = PTHREAD_MUTEX_INITIALIZER;

int portctrl_init(unsigned int base, unsigned int num)
{
    if (num > 65536) {
	return 0;
    }

    if ((ports = (char *) malloc(num))) {
	memset(ports, 0, num);
	ports_max = num;
	ports_base = base;
	return 1;
    }

    return 0;
}

unsigned int portctrl_alloc()
{
    int port = 0;

    pthread_mutex_lock(&ports_mutex);

    for (unsigned int i = 0; i < ports_max; i++) {
	if (!ports[i]) {
	    port = i + ports_base;
	    if (port != 0) {
		ports[i] = 1;
	    }
	    break;
	}
    }

    pthread_mutex_unlock(&ports_mutex);

    return port;
}

int portctrl_free(unsigned int num)
{
    int ret = 0;

    pthread_mutex_lock(&ports_mutex);

    if (num != 0 && (num - ports_base < ports_max)) {
	ports[num - ports_base] = 0;
	ret = 1;
    }

    pthread_mutex_unlock(&ports_mutex);

    return ret;
}

void portctrl_finish()
{
    if (!ports) {
	return;
    }

    for (unsigned int i = 0; i < ports_max; i++) {
	if (ports[i]) {
	    fprintf(stderr, "WARNING: port %d still allocated!\n", ports_base + i);
	}
    }

    free(ports);
}
