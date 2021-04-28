#include <stdio.h>
#include <ctype.h>
#include <simple-connection/tcp.h>
#include "messaging.h"

#ifdef _WIN32
static char *strndup (const char *s, size_t n)
{
    char *result;
    size_t len = strlen (s);

    if (n < len) {
	len = n;
    }

    result = (char *) malloc (len + 1);
    if (!result) {
	return NULL;
    }

    result[len] = '\0';
    return (char *) memcpy (result, s, len);
}
#endif

static int parse_url(char *link, char **scheme, char **host, uint16_t *port, char **path)
{
    if (scheme) {
	*scheme = NULL;
    }
    if (host) {
	*host = NULL;
    }
    if (port) {
	*port = 0;
    }
    if (path) {
	*path = NULL;
    }

    if (!link || strlen(link) == 0) {
	return 1;
    }

    int ret = 0;

    char _link[strlen(link) + 1];
    strcpy(_link, link);

    char *ptr = _link;
    char *endptr = strstr(ptr, "://");
    if (endptr) {
	if (scheme) {
	    *scheme = strndup(ptr, endptr - ptr + 3);
	}
	ptr = endptr + 3;
    }

    endptr = ptr;
    while (*endptr && *endptr != ':' && *endptr != '/') {
	endptr++;
    }

    if (endptr != ptr) {
	if (host) {
	    *host = strndup(ptr, endptr - ptr);
	}
    }

    ptr = endptr;

    if (*ptr == ':') {
	ptr++;
	unsigned int p = strtol(ptr, &endptr, 10);
	if (p == 0 || p >= USHRT_MAX || (errno == ERANGE && (p == USHRT_MAX || p == 0))
		|| (errno != 0 && port == 0) || (ptr == endptr && p == 0)) {
	    fprintf(stderr, "%s: port error '%s'\n", __func__, ptr);
	    ret = 1;
	} else {
	    if (port) {
		*port = p;
	    }
	}
	ptr = endptr;
    }

    if (*ptr == '/') {
	if (*ptr) {
	    if (path) {
		*path = strdup(ptr);
	    }
	}
    } else {
	*path = strdup("/");
    }

    if (port && scheme) {
	if (*port == 0) {
	    if (*scheme && (!strcmp(*scheme, "ws://") || !strcmp(*scheme, "http://"))) {
		*port = 80;
	    } else if (*scheme && (!strcmp(*scheme, "wss://") || !strcmp(*scheme, "https://"))) {
		*port = 443;
	    } else {
		*port = 9998;
	    }
	}
    }

    return ret;
}

int message_send(char *host, char *msg)
{
    int ret = 0;
    char *scheme = NULL;
    char *remote_host = NULL;
    uint16_t remote_port = 0;
    char *path = NULL;
    int use_ssl = 0;
    tcp_channel *server = NULL;

    if (parse_url(host, &scheme, &remote_host, &remote_port, &path)) {
	fprintf(stderr, "Error parsing remote url %s\n", host);
	goto err;
    }

    if (!scheme || (!(!strcmp(scheme, "http://") || !strcmp(scheme, "https://") || !strcmp(scheme, "ws://") || !strcmp(scheme, "wss://")))) {
	fprintf(stderr, "%s: unknown url scheme %s\n", __func__, scheme);
	goto err;
    }

    if (scheme && (!strcmp(scheme, "https://") || !strcmp(scheme, "wss://"))) {
	use_ssl = 1;
    }

    server = tcp_open(use_ssl ? TCP_SSL_CLIENT : TCP_CLIENT, remote_host, remote_port, NULL, NULL);
    if (!server) {
	fprintf(stderr, "%s: tcp_open()\n", __func__);
	goto err;
    }

    if (scheme &&
	(!strcmp(scheme, "ws://") || (!strcmp(scheme, "wss://"))) &&
	(!tcp_connection_upgrade(server, SIMPLE_CONNECTION_METHOD_WS, path, NULL, 0))) {
	fprintf(stderr, "%s: http ws method error!\n", __func__);
	goto err;
    } else {
	char http_req[2048];

	snprintf(http_req, sizeof(http_req), "POST %s?action=message HTTP/1.0\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %ld\r\n\r\n", path, remote_host, strlen(msg));

	fprintf(stderr, "Message send to %s:%d [%s]\n", remote_host, remote_port, http_req);

	if (tcp_write(server, http_req, strlen(http_req)) != strlen(http_req)) {
	    fprintf(stderr, "%s: tcp_write()\n", __func__);
	    goto err;
	}
    }

    int len;
    if (tcp_write(server, msg, strlen(msg)) != strlen(msg)) {
	fprintf(stderr, "%s: tcp_write()\n", __func__);
    } else {
	char tmp[2048];
	if ((len = tcp_read(server, tmp, sizeof(tmp))) > 0) {
	    tmp[len] = 0;
	    fprintf(stderr, "Message from server [%s]\n", tmp);
	    char *msg = "HTTP/1.0 200 OK";
	    if (!strncmp(tmp, msg, strlen(msg))) {
		ret = 1;
	    }
	}
    }

err:
    if (server) {
	tcp_close(server);
    }
    if (scheme) {
	free(scheme);
    }
    if (remote_host) {
	free(remote_host);
    }
    if (path) {
	free(path);
    }

    return ret;
}
