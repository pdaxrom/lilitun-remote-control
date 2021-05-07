/*
 * TCP server
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#ifndef _WIN32
#include <sys/select.h>
#endif

#include "../thirdparty/jsmn/jsmn.h"

#include "main.h"

static int is_not_exit = 1;

static char *remote_id = REMOTE_ID;
static char *server_id = SERVER_ID;
static char *client_id = CLIENT_ID;

static struct list_item *remote_connections_list = NULL;
static pthread_mutex_t remote_connections_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static int send_uint32(tcp_channel * channel, uint32_t data)
{
    uint32_t tmp = htonl(data);
    if (tcp_write(channel, (char *)&tmp, 4) != 4) {
	return 0;
    }

    return 1;
}

static int recv_uint32(tcp_channel * channel, uint32_t * data)
{
    int timeout = select_read_timeout(tcp_fd(channel), 5);
    if (timeout == 0) {
	fprintf(stderr, "Timeout %d\n", timeout);
	return 0;
    }

    uint32_t tmp;
    if (tcp_read(channel, (char *)&tmp, 4) != 4) {
	return 0;
    }

    *data = ntohl(tmp);
    return 1;
}

static int tcp_read_all(tcp_channel * channel, char *buf, int len)
{
    int total = 0;

    while (total != len) {
	int ret = tcp_read(channel, buf + total, len - total);
	if (ret <= 0) {
	    break;
	} else {
	    total += ret;
	}
    }

    return total;
}

static int check_remote_sig(tcp_channel * channel)
{
    int ret = 0;
    int r;
    char buf[256];

    if ((r = tcp_read_all(channel, buf, strlen(remote_id) + 1)) != (strlen(remote_id) + 1)) {
	fprintf(stderr, "%s: tcp_read()\n", __func__);
	return 0;
    }

    fprintf(stderr, "buf[%d]=%s\n", r, buf);

    if (!strcmp(buf, remote_id)) {
	ret = 1;
    }

    if (!strcmp(buf, client_id)) {
	ret = 2;
    }

    if (!ret) {
	return 0;
    }

    if ((r = tcp_write(channel, server_id, strlen(server_id) + 1)) <= 0) {
	fprintf(stderr, "%s: tcp_write()\n", __func__);
	return 0;
    }

    return ret;
}

static int do_req_initial(tcp_channel * channel, uint32_t req)
{
    uint32_t tmp = REQ_ERROR;

    if (!send_uint32(channel, req)) {
	return 0;
    }

    if (!recv_uint32(channel, &tmp)) {
	return 0;
    }

    if (tmp != req) {
	return 0;
    }

    return 1;
}

static char *do_req_appserver_host(tcp_channel * channel)
{
    if (!do_req_initial(channel, REQ_APPSERVER_HOST)) {
	return NULL;
    }

    uint32_t len;

    if (!recv_uint32(channel, &len)) {
	return NULL;
    }

    char *str = (char *) malloc(len + 1);

    if (tcp_read_all(channel, str, len) != len) {
	free(str);
	return NULL;
    }

    str[len] = 0;

    return str;
}

static char *do_req_session_id(tcp_channel * channel)
{
    if (!do_req_initial(channel, REQ_SESSION_ID)) {
	return NULL;
    }

    uint32_t len;

    if (!recv_uint32(channel, &len)) {
	return NULL;
    }

    char *str = (char *) malloc(len + 1);

    if (tcp_read_all(channel, str, len) != len) {
	free(str);
	return NULL;
    }

    str[len] = 0;

    return str;
}

static char *do_req_hostname(tcp_channel * channel)
{
    if (!do_req_initial(channel, REQ_HOSTNAME)) {
	return NULL;
    }

    uint32_t len;

    if (!recv_uint32(channel, &len)) {
	return NULL;
    }

    char *str = (char *) malloc(len + 1);

    if (tcp_read_all(channel, str, len) != len) {
	free(str);
	return NULL;
    }

    str[len] = 0;

    return str;
}

static int do_req_stop(tcp_channel *channel)
{
    if (!send_uint32(channel, REQ_STOP)) {
	return 0;
    }

    return 1;
}

static int send_user_password(tcp_channel * channel, char * password)
{
    if (!send_uint32(channel, REQ_USER_PASSWORD)) {
	return 0;
    }

    uint32_t len = strlen(password);

    if (!send_uint32(channel, len)) {
	return 0;
    }

    if (tcp_write(channel, password, len) != len) {
	return 0;
    }

    return 1;
}

static int send_user_connection(tcp_channel * channel, int port, int ipv6port)
{
    if (!send_uint32(channel, REQ_USER_CONNECTION)) {
	return 0;
    }

    uint32_t tmp = port;

    if (!send_uint32(channel, tmp)) {
	return 0;
    }

    tmp = ipv6port;

    if (!send_uint32(channel, tmp)) {
	return 0;
    }

    return 1;
}

static void *remote_connection_thread(void *arg)
{
    struct remote_connection_t *conn = (struct remote_connection_t *) arg;

    pthread_detach(pthread_self());

    if (!tcp_connection_upgrade(conn->channel, SIMPLE_CONNECTION_METHOD_WS, "/projector-ws", NULL, 0)) {
	fprintf(stderr, "%s: http ws method error!\n", __func__);
	tcp_close(conn->channel);
	free(conn);
	return NULL;
    }

    conn->thread_alive = 0;
    conn->stop_req = 0;

    pthread_mutex_init(&conn->projector_io_mutex, NULL);

    pthread_mutex_lock(&remote_connections_list_mutex);
    list_add_data(&remote_connections_list, conn);
    pthread_mutex_unlock(&remote_connections_list_mutex);

    if ((conn->type = check_remote_sig(conn->channel))) {
	fprintf(stderr, "remote connection thread started, type = %s\n!\n", (type == 1) ? "Remote app" : "Client app");
	if (conn->type == 1) {
	    start_remote_app_session(conn);
	} else {
	    start_client_session(conn);
	}
    }

    fprintf(stderr, "remote connection thread finished!\n");

    if (conn->apphost) {
	free(conn->apphost);
    }

    if (conn->session_id) {
	free(conn->session_id);
    }

    if (conn->hostname) {
	free(conn->hostname);
    }

    tcp_close(conn->channel);

    pthread_mutex_lock(&remote_connections_list_mutex);
    list_remove_data(&remote_connections_list, conn);
    pthread_mutex_unlock(&remote_connections_list_mutex);

    pthread_mutex_destroy(&conn->projector_io_mutex);

    free(conn);

    return NULL;
}

static void *stop_sharing(char *session_id)
{
    pthread_mutex_lock(&remote_connections_list_mutex);

    struct list_item *item = remote_connections_list;

    while (item) {
	struct remote_connection_t *conn = list_get_data(item);
	pthread_mutex_lock(&conn->projector_io_mutex);
	if (!strcmp(conn->session_id, session_id)) {
	    fprintf(stderr, "stop sharing sessionId=%s\n", session_id);
	    conn->stop_req = 1;
	    pthread_mutex_unlock(&conn->projector_io_mutex);
	    break;
	}
	pthread_mutex_unlock(&conn->projector_io_mutex);
	item = list_next_item(item);
    }

    pthread_mutex_unlock(&remote_connections_list_mutex);
}

static int get_http_header(tcp_channel *channel, char *head, int headLen)
{
    int nread;
    int totalRead = 0;
    char crlf[4] = { 0, 0, 0, 0 };

    while (totalRead < headLen - 1) {
	nread = tcp_read(channel, &head[totalRead], 1);
	if (nread <= 0) {
//		fprintf(stderr, "%s: Cannot read response (%s)\n", __func__, strerror(errno));
	    return -1;
	}
	crlf[0] = crlf[1];
	crlf[1] = crlf[2];
	crlf[2] = crlf[3];
	crlf[3] = head[totalRead];

	totalRead += nread;

	if (crlf[0] == 13 && crlf[1] == 10 && crlf[2] == 13 && crlf[3] == 10) {
	    break;
	    }

	if (crlf[2] == 10 && crlf[3] == 10) {
	    break;
	}
    }

    head[totalRead] = 0;

    return totalRead;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
	strncmp(json + tok->start, s, tok->end - tok->start) == 0) {

	return 0;
    }

    return -1;
}

static void *control_connection_thread(void *arg)
{
    struct control_connection_t *conn = (struct control_connection_t *) arg;

    pthread_detach(pthread_self());

    char req[1024];

    char *resp = NULL;

    fprintf(stderr, "control connection started\n");

    if (get_http_header(conn->channel, req, sizeof(req)) > 0) {
	fprintf(stderr, "Control message %s\n", req);

	if (!strncmp(req, "POST /", 5)) {
	    char *action = request_parse(&req[5], "action", NULL, 0);

	    if (!strcmp(action, "message")) {
		int len;
		if ((len = tcp_read(conn->channel, req, sizeof(req) - 1)) > 0) {
		    jsmn_parser p;
		    jsmntok_t t[16];

		    req[len] = 0;

		    fprintf(stderr, "control message: %s\n", req);

		    jsmn_init(&p);
		    int r = jsmn_parse(&p, req, strlen(req), t,
				sizeof(t) / sizeof(t[0]));
		    if (r < 0) {
			fprintf(stderr, "Failed to parse JSON: %d\n", r);
			resp = "HTTP/1.1 403 Forbidden\r\n\r\n";
		    } else {
			if (r < 1 || t[0].type != JSMN_OBJECT) {
			    fprintf(stderr, "Object expected\n");
			    resp = "HTTP/1.1 403 Forbidden\r\n\r\n";
			} else {
			    char *request_type = NULL;
			    char *session_id = NULL;

			    for (int i = 1; i < r; i++) {
				if (jsoneq(req, &t[i], "requestType") == 0) {
				    request_type = strndup(req + t[i + 1].start, t[i + 1].end - t[i + 1].start);
				    i++;
				} else if (jsoneq(req, &t[i], "sessionId") == 0) {
				    session_id = strndup(req + t[i + 1].start, t[i + 1].end - t[i + 1].start);
				    i++;
				}
			    }

			    if (request_type) {
				fprintf(stderr, "requestType : %s\n", request_type);
			    }
			    if (session_id) {
				fprintf(stderr, "sessionId   : %s\n", session_id);
			    }

			    if (!strcmp(request_type, "stopSharing") && session_id) {
				stop_sharing(session_id);
			    }

			    if (session_id) {
				free(session_id);
			    }
			    if (request_type) {
				free(request_type);
			    }
			    resp = "HTTP/1.1 200 OK";
			}
		    }
		}
	    } else {
		resp = "HTTP/1.1 403 Forbidden\r\n\r\n";
	    }

	    if (tcp_write(conn->channel, resp, strlen(resp)) != strlen(resp)) {
		fprintf(stderr, "%s: tcp_write()\n", __func__);
	    }

	    if (action) {
		free(action);
	    }
	}
    }

    tcp_close(conn->channel);
    free(conn);

    fprintf(stderr, "control connection finished\n");

    return NULL;
}

static void *control_thread(void *arg)
{
    struct control_t *control = (struct control_t *) arg;

    pthread_detach(pthread_self());

    fprintf(stderr, "start control thread\n");

    tcp_channel *server = tcp_open(control->use_ssl ? TCP_SSL_SERVER : TCP_SERVER, NULL, control->port, control->sslkeyfile, control->sslcertfile);
    if (!server) {
	fprintf(stderr, "tcp_open()\n");
	is_not_exit = 0;
	return NULL;
    }

    while (is_not_exit) {
	fd_set fds;
	int res;
	struct timeval tv = { 0, 500000 };
	pthread_t tid;

	FD_ZERO(&fds);
	FD_SET(tcp_fd(server), &fds);
	res = select(tcp_fd(server) + 1, &fds, NULL, NULL, &tv);
	if (res < 0) {
	    fprintf(stderr, "%s select()\n", __func__);
	    break;
	}

	if (!res) {
	    continue;
	}

	if (!FD_ISSET(tcp_fd(server), &fds)) {
	    continue;
	}

	tcp_channel *client = tcp_accept(server);
	if (!client) {
	    fprintf(stderr, "tcp_accept()\n");
	    continue;
	}

	struct control_connection_t *arg = malloc(sizeof(struct control_connection_t));
	memset(arg, 0, sizeof(struct control_connection_t));
	arg->channel = client;

	if (pthread_create(&tid, NULL, &control_connection_thread, (void *)arg) != 0) {
	    fprintf(stderr, "%s pthread_create(remote_connection_thread)\n", __func__);
	    free(arg);
	    tcp_close(client);
	}
    }

    tcp_close(server);

    free(control);

    return NULL;
}

static void exit_signal()
{
    fprintf(stderr, "exit signal\n");

    is_not_exit = 0;
}

int main(int argc, char *argv[])
{
    char *sslcertfile = NULL;
    char *sslkeyfile = NULL;
    int control_use_ssl;
    int control_port;
    int projector_use_ssl;
    int projector_port;
    int client_use_ssl;
    pthread_t control_tid;

    /* Ignore PIPE signal and return EPIPE error */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, exit_signal);

    Config *conf = ConfigOpen((argc > 1) ? argv[1] : CONFIG_DIR "/controlserver.conf");
    if (!conf) {
	fprintf(stderr, "Cannot open config file!\n");
	return -1;
    }

    sslcertfile = ConfigReadString(conf, "certfile", NULL, 0, NULL);
    if (!sslcertfile) {
	fprintf(stderr, "Add certificate file path to config!\n");
	return -1;
    }

    sslkeyfile = ConfigReadString(conf, "keyfile", NULL, 0, NULL);
    if (!sslkeyfile) {
	fprintf(stderr, "Add private key file path to config!\n");
	return -1;
    }

    control_use_ssl = ConfigReadInt(conf, "controlusessl", 0);
    projector_use_ssl = ConfigReadInt(conf, "projectorusessl", 1);

    control_port = ConfigReadInt(conf, "controlport", 9996);
    projector_port = ConfigReadInt(conf, "projectorport", 80);

    ConfigClose(conf);

    fprintf(stderr, "SSL cert file     : %s\n", sslcertfile);
    fprintf(stderr, "SSL key file      : %s\n", sslkeyfile);
    fprintf(stderr, "Control use ssl   : %d\n", control_use_ssl);
    fprintf(stderr, "Control port      : %d\n", control_port);
    fprintf(stderr, "Projector use ssl : %d\n", projector_use_ssl);
    fprintf(stderr, "Projector port    : %d\n", projector_port);

    fprintf(stderr, "start control server\n");

    struct control_t *control = (struct control_t *) malloc(sizeof(struct control_t));
    control->port = control_port;
    control->use_ssl = control_use_ssl;
    control->sslcertfile = sslcertfile;
    control->sslkeyfile = sslkeyfile;

    if (pthread_create(&control_tid, NULL, &control_thread, (void *) control) != 0) {
	fprintf(stderr, "%s pthread_create(control_thread)\n", __func__);
	free(control);
	free(sslcertfile);
	free(sslkeyfile);
	return -1;
    }

    tcp_channel *server = tcp_open(projector_use_ssl ? TCP_SSL_SERVER : TCP_SERVER, NULL, projector_port, sslkeyfile, sslcertfile);
    if (!server) {
	fprintf(stderr, "tcp_open()\n");
	return -1;
    }

    while (is_not_exit) {
	fd_set fds;
	int res;
	struct timeval tv = { 0, 500000 };

	FD_ZERO(&fds);
	FD_SET(tcp_fd(server), &fds);
	res = select(tcp_fd(server) + 1, &fds, NULL, NULL, &tv);
	if (res < 0) {
	    fprintf(stderr, "%s select()\n", __func__);
	    break;
	}

	if (!res) {
	    continue;
	}

	if (!FD_ISSET(tcp_fd(server), &fds)) {
	    continue;
	}

	tcp_channel *client = tcp_accept(server);
	if (!client) {
	    fprintf(stderr, "tcp_accept()\n");
	    continue;
	}

	struct remote_connection_t *arg = malloc(sizeof(struct remote_connection_t));
	memset(arg, 0, sizeof(struct remote_connection_t));
	arg->channel = client;
	arg->sslcertfile = sslcertfile;
	arg->sslkeyfile = sslkeyfile;
	arg->use_ssl = projector_use_ssl;
	arg->client_use_ssl = client_use_ssl;

	if (pthread_create(&arg->tid, NULL, &remote_connection_thread, (void *)arg) != 0) {
	    fprintf(stderr, "%s pthread_create(remote_connection_thread)\n", __func__);
	    free(arg);
	    tcp_close(client);
	}
    }

    tcp_close(server);

    if (sslkeyfile) {
	free(sslkeyfile);
    }

    if (sslcertfile) {
	free(sslcertfile);
    }

    fprintf(stderr, "stop control server\n");

    return 0;
}
