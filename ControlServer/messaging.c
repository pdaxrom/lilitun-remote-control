#include <stdio.h>
#include <ctype.h>
#include <simple-connection/tcp.h>
#include "messaging.h"

int message_send(char *host, char *msg)
{
    char *path = "/";
    char *remote_host = strdup(host);
    if (!remote_host) {
	return 0;
    }
    int remote_port = 443;
    if (strchr(remote_host, ':')) {
//	char *endptr = remote_host + strlen(remote_host);
	char *ptr = strchr(remote_host, ':');
	*ptr++ = 0;

	char *endptr = ptr;
	while (*endptr && isdigit(*endptr)) {
	    endptr++;
	}

	if (ptr != endptr && *endptr) {
	    path = alloca(strlen(endptr) + 1);
	    strcpy(path, endptr);
	    *endptr = 0;
	}

	if (ptr) {
	    unsigned int port = strtol(ptr, &endptr, 10);
	    if (port == 0 || port >= USHRT_MAX || (errno == ERANGE && (port == USHRT_MAX || port == 0))
		    || (errno != 0 && port == 0)) {
		fprintf(stderr, "controlServer port error '%s'", ptr);
	    } else {
		remote_port = port;
	    }
	}
    } else if (strchr(remote_host, '/')) {
	char *ptr = strchr(remote_host, '/');
	path = alloca(strlen(ptr) + 1);
	strcpy(path, ptr);
	*ptr = 0;
    }

    tcp_channel *server = tcp_open(TCP_SSL_CLIENT, remote_host, remote_port, NULL, NULL);
    if (!server) {
	fprintf(stderr, "%s: tcp_open()\n", __func__);
	free(remote_host);
	return 0;
    }

    int ret = 0;
    char http_req[2048];

    snprintf(http_req, sizeof(http_req), "POST %s?action=message HTTP/1.0\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %ld\r\n\r\n", path, remote_host, strlen(msg));

    fprintf(stderr, "Message send to %s:%d [%s]\n", remote_host, remote_port, http_req);

    if (tcp_write(server, http_req, strlen(http_req)) != strlen(http_req)) {
	fprintf(stderr, "%s: tcp_write()\n", __func__);
    } else {
	int len;
	if (tcp_write(server, msg, strlen(msg)) != strlen(msg)) {
	    fprintf(stderr, "%s: tcp_write()\n", __func__);
	} else {
	    if ((len = tcp_read(server, http_req, sizeof(http_req))) > 0) {
		http_req[len] = 0;
		fprintf(stderr, "Message from server [%s]\n", http_req);
		char *msg = "HTTP/1.0 200 OK";
		if (!strncmp(http_req, msg, strlen(msg))) {
		    ret = 1;
		}
	    }
	}
    }
    tcp_close(server);
    free(remote_host);

    return ret;
}
