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

static char *client_id = CLIENT_ID;
static char *server_id = SERVER_ID;

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
    int r;
    char buf[256];

    if ((r = tcp_read_all(channel, buf, strlen(client_id) + 1)) != (strlen(client_id) + 1)) {
	fprintf(stderr, "%s: tcp_read()\n", __func__);
	return 0;
    }

    fprintf(stderr, "buf[%d]=%s\n", r, buf);
    if (strcmp(buf, client_id)) {
	return 0;
    }

    if ((r = tcp_write(channel, server_id, strlen(client_id) + 1)) <= 0) {
	fprintf(stderr, "%s: tcp_write()\n", __func__);
	return 0;
    }

    return 1;
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

int do_req_screen_info(tcp_channel *channel, struct screen_info_t *screen_info)
{
    if (!do_req_initial(channel, REQ_SCREEN_INFO)) {
	return 0;
    }

    if (!recv_uint32(channel, &screen_info->width)) {
	return 0;
    }

    if (!recv_uint32(channel, &screen_info->height)) {
	return 0;
    }

    if (!recv_uint32(channel, &screen_info->depth)) {
	return 0;
    }

    if (!recv_uint32(channel, &screen_info->pixelformat)) {
	return 0;
    }

    return 1;
}

static int do_req_screen_update(tcp_channel *channel, uint32_t *regions)
{
    if (!do_req_initial(channel, REQ_SCREEN_UPDATE)) {
	return 0;
    }

    if (!recv_uint32(channel, regions)) {
	return 0;
    }

    return 1;
}

static int do_req_screen_update_region(tcp_channel * channel, uint32_t * x, uint32_t * y, uint32_t * width, uint32_t * height,
				uint32_t * pixfmt, uint32_t * length)
{
    if (!recv_uint32(channel, x)) {
	return 0;
    }

    if (!recv_uint32(channel, y)) {
	return 0;
    }

    if (!recv_uint32(channel, width)) {
	return 0;
    }

    if (!recv_uint32(channel, height)) {
	return 0;
    }

    if (!recv_uint32(channel, pixfmt)) {
	return 0;
    }

    if (!recv_uint32(channel, length)) {
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

static void send_input_events(struct remote_connection_t *conn)
{
    for (int i = 0; i < conn->input_events_counter; i++) {
	send_uint32(conn->channel, conn->input_events[i]);
    }

    conn->input_events_counter = 0;
    conn->pointer_old.buttons = -1;
}

static void add_input_event(struct remote_connection_t *conn, uint32_t data)
{
    conn->input_events[conn->input_events_counter++] = data;
}

static void keyboard_event(signed char down, unsigned int key, struct _rfbClientRec *cl)
{
    struct remote_connection_t *conn = cl->screen->screenData;
//    fprintf(stderr, "keyboard event %d\n", down);

    pthread_mutex_lock(&conn->projector_io_mutex);

    if (conn->input_events_counter >= (sizeof(conn->input_events) - sizeof(req_update_pointer))) {
//fprintf(stderr, "keyboard overflow\n");
	send_input_events(conn);
    }

    add_input_event(conn, REQ_KEYBOARD);
    add_input_event(conn, down);
    add_input_event(conn, key);

    pthread_mutex_unlock(&conn->projector_io_mutex);
}

static void pointer_event(int buttons, int x, int y, struct _rfbClientRec *cl)
{
    struct remote_connection_t *conn = cl->screen->screenData;
//    fprintf(stderr, "pointer event %d %d, %d\n", buttons, x, y);

    pthread_mutex_lock(&conn->projector_io_mutex);

    if (conn->input_events_counter >= (sizeof(conn->input_events) - sizeof(req_update_pointer))) {
//fprintf(stderr, "pointer overflow\n");
	send_input_events(conn);
    }

    if (conn->pointer_old.buttons != buttons) {
	add_input_event(conn, REQ_POINTER);
	add_input_event(conn, buttons);
	add_input_event(conn, x);
	add_input_event(conn, y);
	add_input_event(conn, 0);

	conn->pointer_old.buttons = buttons;
	conn->pointer_old.pos_x = x;
	conn->pointer_old.pos_y = y;
	conn->pointer_old.wheel = 0;
    }

    pthread_mutex_unlock(&conn->projector_io_mutex);
}

static void *remote_connection_thread(void *arg)
{
    struct remote_connection_t *conn = (struct remote_connection_t *) arg;

    pthread_detach(pthread_self());

    if (!tcp_connection_upgrade(conn->channel, SIMPLE_CONNECTION_METHOD_WS, "/projector-ws")) {
	fprintf(stderr, "%s: http ws method error!\n", __func__);
	tcp_close(conn->channel);
	free(conn);
	return NULL;
    }

    pthread_mutex_init(&conn->framebuffer_mutex, NULL);
    pthread_mutex_init(&conn->projector_io_mutex, NULL);

    conn->thread_alive = 0;

    pthread_mutex_lock(&remote_connections_list_mutex);
    list_add_data(&remote_connections_list, conn);
    pthread_mutex_unlock(&remote_connections_list_mutex);

    if (check_remote_sig(conn->channel)) {
	fprintf(stderr, "remote connection thread started!\n");
	if (do_req_screen_info(conn->channel, &conn->screen_info)) {
	    fprintf(stderr, "remote screen %dx%d depth %d pixelformat %d\n",
		    conn->screen_info.width, conn->screen_info.height, conn->screen_info.depth,
		    conn->screen_info.pixelformat);
	    conn->framebuffer =
		(uint8_t *) malloc(conn->screen_info.width * conn->screen_info.height * (conn->screen_info.depth / 8));
	    memset(conn->framebuffer, 0,
		   conn->screen_info.width * conn->screen_info.height * (conn->screen_info.depth / 8));

	    conn->host = NULL;
	    conn->httpdir = NULL;
//	    conn->httpdir = WEBROOT_DIR;
//	    conn->sslkeyfile = CONFIG_DIR "/privkey1.pem";
//	    conn->sslcertfile = CONFIG_DIR "/cert1.pem";
	    conn->port = portctrl_alloc();
	    conn->ipv6port = portctrl_alloc();
	    conn->httpport = -1;
	    conn->http6port = -1;
//	    conn->httpport = portctrl_alloc();
//	    conn->http6port = portctrl_alloc();

	    conn->passwds = malloc(sizeof(char**)*2);
	    conn->passwds[0] = get_random_string(12);
	    conn->passwds[1] = NULL;

	    conn->apphost = NULL;
	    conn->session_id = NULL;
	    conn->hostname = NULL;

	    pthread_mutex_lock(&conn->projector_io_mutex);
	    if ((conn->apphost = do_req_appserver_host(conn->channel)) &&
		(conn->session_id = do_req_session_id(conn->channel)) &&
		(conn->hostname = do_req_hostname(conn->channel)) &&
		send_user_connection(conn->channel, conn->port, conn->ipv6port) &&
		send_user_password(conn->channel, conn->passwds[0])) {
		if ((conn->translator = translator_init(conn, keyboard_event, pointer_event))) {
		    fprintf(stderr, "apphost %s\n", conn->apphost);
		    fprintf(stderr, "session id %s\n", conn->session_id);
		    fprintf(stderr, "hostname %s\n", conn->hostname);
		    fprintf(stderr, "remote users passwd %s\n", conn->passwds[0]);
		    conn->thread_alive = 1;
		    char msg[1024];
		    snprintf(msg, sizeof(msg), "{\"requestType\":\"remoteControlStart\",\"sessionId\":\"%s\",\"hostname\":\"%s\",\"port\":\"%d\",\"ipv6port\":\"%d\",\"password\":\"%s\"}",
				conn->session_id, conn->hostname, conn->port, conn->ipv6port, conn->passwds[0]);
		    if (!message_send(conn->apphost, msg)) {
			fprintf(stderr, "Appserver unavailable, close remote sharing!\n");
			conn->thread_alive = 0;
		    }
		}
	    }
	    pthread_mutex_unlock(&conn->projector_io_mutex);
	}

	while (is_not_exit && conn->thread_alive) {
#ifdef DEBUG_CLIENT_LOOP
	    fprintf(stderr, "send screen update request\n");
#endif
	    pthread_mutex_lock(&conn->projector_io_mutex);

	    send_input_events(conn);

	    uint32_t regions;

	    if (!do_req_screen_update(conn->channel, &regions)) {
		conn->thread_alive = 0;

		pthread_mutex_unlock(&conn->projector_io_mutex);
		break;
	    }

	    while (regions-- > 0) {
		uint32_t x, y, width, height, pixfmt, pixlen, rlen;

		if (!do_req_screen_update_region(conn->channel, &x, &y, &width, &height, &pixfmt, &pixlen)) {
		    conn->thread_alive = 0;

		    pthread_mutex_unlock(&conn->projector_io_mutex);
		    break;
		}

//fprintf(stderr, "x=%d y=%d w=%d h=%d pixfmt=%d pixlen=%d\n", x, y, width, height, pixfmt, pixlen);

		char pixdata[pixlen];

		if (pixlen > 0) {
		    if ((rlen = tcp_read_all(conn->channel, pixdata, pixlen)) != pixlen) {
			conn->thread_alive = 0;
			fprintf(stderr, "Error reading framebuffer (%d of %d)\n", rlen, pixlen);

			pthread_mutex_unlock(&conn->projector_io_mutex);
			break;
		    }
		}

		pthread_mutex_unlock(&conn->projector_io_mutex);

#ifdef DEBUG_CLIENT_LOOP
		fprintf(stderr, "screen update %d x %d - %d x %d bpp=%d length=%d\n", x, y, width, height, bpp, jpeglen);
#endif

		if (pixlen > 0) {
		    unsigned char *rawImage = NULL;
		    unsigned int rawWidth;
		    unsigned int rawHeight;
		    int rawPixelSize;

		    if (pixfmt == PIX_JPEG_BGRA || pixfmt == PIX_JPEG_RGBA) {
			if (decompress_jpeg_to_raw
			    ((unsigned char *)pixdata, pixlen, &rawImage, &rawWidth, &rawHeight, &rawPixelSize)) {
			    conn->thread_alive = 0;
			    fprintf(stderr, "error decompressing jpeg\n");
			    break;
			}
		    } else if (pixfmt == PIX_LZ4_BGRA || pixfmt == PIX_LZ4_RGBA) {
			int rawLen = rawWidth * rawHeight * 4;
			rawImage = (unsigned char *) malloc(rawLen);
			if (LZ4_decompress_safe(pixdata, rawImage, pixlen, rawLen) <= 0) {
			    free(rawImage);
			    conn->thread_alive = 0;
			    fprintf(stderr, "error decompressing lz4\n");
			    break;
			}
			rawWidth = width;
			rawHeight = height;
			rawPixelSize = 4;
		    } else if (pixfmt == PIX_RAW_BGRA || pixfmt == PIX_RAW_RGBA) {
			rawImage = pixdata;
			rawWidth = width;
			rawHeight = height;
			rawPixelSize = 4;
		    }
#ifdef DEBUG_CLIENT_LOOP
		    fprintf(stderr, "preparing image %dx%dx%d\n", rawWidth, rawHeight, rawPixelSize);
#endif

		    pthread_mutex_lock(&conn->framebuffer_mutex);
		    fbops_bitblit(conn->framebuffer, conn->screen_info.width, conn->screen_info.height,
				    conn->screen_info.depth, rawImage, x, y, rawWidth, rawHeight, rawPixelSize * 8, pixfmt);
		    pthread_mutex_unlock(&conn->framebuffer_mutex);

		    if (!(pixfmt == PIX_RAW_BGRA || pixfmt == PIX_RAW_RGBA)) {
			free(rawImage);
		    }
		}
	    }
	    usleep(50000);
	}
    }

    if (conn->translator) {
	char msg[1024];
	snprintf(msg, sizeof(msg), "{\"requestType\":\"remoteControlStop\",\"sessionId\":\"%s\"}",
		conn->session_id);
	if (!message_send(conn->apphost, msg)) {
	    fprintf(stderr, "Appserver unavailable!\n");
	}

	portctrl_free(conn->port);
	portctrl_free(conn->ipv6port);
//	portctrl_free(conn->httpport);
//	portctrl_free(conn->http6port);
	translator_finish(conn->translator);
    }

    fprintf(stderr, "remote connection thread finished!\n");

    if (conn->passwds) {
	for(char **passwd = conn->passwds; *passwd; passwd++) {
	    free(*passwd);
	}

	free(conn->passwds);
    }

    if (conn->apphost) {
	free(conn->apphost);
    }

    if (conn->session_id) {
	free(conn->session_id);
    }

    if (conn->hostname) {
	free(conn->hostname);
    }

    if (conn->framebuffer) {
	free(conn->framebuffer);
    }

    tcp_close(conn->channel);

    pthread_mutex_lock(&remote_connections_list_mutex);
    list_remove_data(&remote_connections_list, conn);
    pthread_mutex_unlock(&remote_connections_list_mutex);

    pthread_mutex_destroy(&conn->projector_io_mutex);
    pthread_mutex_destroy(&conn->framebuffer_mutex);

    free(conn);

    return NULL;
}

static void *stop_sharing(char *session_id)
{
    pthread_mutex_lock(&remote_connections_list_mutex);

    struct list_item *item = remote_connections_list;

    while (item) {
	struct remote_connection_t *conn = list_get_data(item);
	if (!strcmp(conn->session_id, session_id)) {
		fprintf(stderr, "stop sharing sessionId=%s\n", session_id);
		conn->thread_alive = 0;
		break;
	}
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
    int client_port;
    int client_port_size;
    pthread_t control_tid;

    /* Ignore PIPE signal and return EPIPE error */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, exit_signal);

    Config *conf = ConfigOpen(CONFIG_DIR "/controlserver.conf");
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
    client_use_ssl = ConfigReadInt(conf, "clientusessl", 0);

    control_port = ConfigReadInt(conf, "controlport", 9996);
    projector_port = ConfigReadInt(conf, "projectorport", 80);
    client_port = ConfigReadInt(conf, "clientport", 7000);
    client_port_size = ConfigReadInt(conf, "clientportsize", 512);

    ConfigClose(conf);

    fprintf(stderr, "SSL cert file     : %s\n", sslcertfile);
    fprintf(stderr, "SSL key file      : %s\n", sslkeyfile);
    fprintf(stderr, "Control use ssl   : %d\n", control_use_ssl);
    fprintf(stderr, "Control port      : %d\n", control_port);
    fprintf(stderr, "Projector use ssl : %d\n", projector_use_ssl);
    fprintf(stderr, "Projector port    : %d\n", projector_port);
    fprintf(stderr, "Client use ssl    : %d\n", client_use_ssl);
    fprintf(stderr, "Client port start : %d\n", client_port);
    fprintf(stderr, "Client port end   : %d\n", client_port + client_port_size - 1);

    fprintf(stderr, "start control server\n");

    if (!portctrl_init(client_port, client_port_size)) {
	fprintf(stderr, "Cannot initialize port control!\n");
	return -1;
    }

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
	arg->framebuffer = NULL;
	arg->translator = NULL;
	arg->passwds = NULL;
	arg->input_events_counter = 0;
	arg->pointer_old.buttons = -1;
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

    portctrl_finish();

    if (sslkeyfile) {
	free(sslkeyfile);
    }

    if (sslcertfile) {
	free(sslcertfile);
    }

    fprintf(stderr, "stop control server\n");

    return 0;
}
