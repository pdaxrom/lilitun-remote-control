#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>

#ifdef _WIN32
#define SIGHUP	1
#define SIGINT	2
#define SIGUSR1	10
#define SIGPIPE	13
#endif

#include "../Common/protos.h"
#include <simple-connection/tcp.h>
#include "xgrabber.h"
#include "jpegenc.h"
#include "getrandom.h"
#include "base64.h"
#include "projector.h"
#include "../thirdparty/lz4/lz4.h"
#include "pseudofs.h"

/*****************************************************************************/

#define BLOCK_WIDTH	128
#define BLOCK_HEIGHT	128

extern void write_log(const char *fmt, ...);

static char *remote_id = REMOTE_ID;
static char *server_id = SERVER_ID;
static char *client_id = CLIENT_ID;

struct region_data {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t pixfmt;
    uint32_t length;
    uint8_t *data;
};

static struct projector_t *init_fb(void)
{
    struct projector_t *projector = (struct projector_t *)malloc(sizeof(struct projector_t));

    memset(projector, 0, sizeof(struct projector_t));

    if (!projector) {
	return NULL;
    }

    projector->xgrabber = GrabberInit(&projector->scrWidth, &projector->scrHeight, &projector->scrDepth);
    if (!projector->xgrabber) {
	free(projector);
	write_log("Cannot init screen capturing!\n");
	return NULL;
    }

    projector->blocks_x = projector->scrWidth / BLOCK_WIDTH;
    projector->blocks_y = projector->scrHeight / BLOCK_HEIGHT;

    projector->blocks_x += (projector->scrWidth > projector->blocks_x * BLOCK_WIDTH) ? 1 : 0;
    projector->blocks_y += (projector->scrHeight > projector->blocks_y * BLOCK_HEIGHT) ? 1 : 0;

    write_log("xres = %d\n", projector->scrWidth);
    write_log("yres = %d\n", projector->scrHeight);
    write_log("bpp  = %d\n", projector->scrDepth * 8);

    return projector;
}

static void dump_file(char *name, char *buf, int len)
{
    FILE *out = fopen(name, "wb");
    if (out) {
	fwrite(buf, 1, len, out);
	fclose(out);
    }
}

static int tcp_write_all(struct projector_t *projector, char *buf, int len)
{
    if (tcp_write(projector->channel, buf, len) != len) {
	write_log("tcp_write()\n");
	return 0;
    }

    return len;
}

static int tcp_read_all(struct projector_t *projector, char *buf, int len)
{
    int total = 0;

    while (total != len) {
	int avail = len - total;

	int ret = tcp_read(projector->channel, buf + total, avail);
	if (ret <= 0) {
	    break;
	} else {
	    total += ret;
	}
    }

    return total;
}

static int send_uint32(struct projector_t *projector, uint32_t data)
{
    uint32_t tmp = htonl(data);
    if (tcp_write_all(projector, (char *)&tmp, 4) != 4) {
	return 0;
    }

    return 1;
}

static int recv_uint32(struct projector_t *projector, uint32_t * data)
{
    uint32_t tmp;
    if (tcp_read_all(projector, (char *)&tmp, 4) != 4) {
	return 0;
    }

    *data = ntohl(tmp);
    return 1;
}

static int check_remote_sig(struct projector_t *projector, int mode)
{
    int r;
    char buf[256];

    if (mode) {
	uint32_t length = 0;

	if (!recv_uint32(projector, &length)) {
	    return 0;
	}

	length = (length > sizeof(buf)) ? sizeof(buf) : length;

	if ((r = tcp_read_all(projector, buf, length)) == length) {
	    buf[r] = 0;
	    write_log("buf[%d]=%s\n", r, buf);
	    if (!strncmp(buf, client_id, length)) {
		if (!send_uint32(projector, strlen(remote_id))) {
		    write_log("tcp_write()\n");
		    return 0;
		}
		if ((r = tcp_write_all(projector, remote_id, strlen(remote_id))) <= 0) {
		    write_log("tcp_write()\n");
		    return 0;
		}
		return 1;
	    }
	}
    } else {
	if ((r = tcp_write_all(projector, remote_id, strlen(remote_id) + 1)) <= 0) {
	    write_log("tcp_write()\n");
	    return 0;
	}

	if ((r = tcp_read_all(projector, buf, strlen(server_id) + 1)) == (strlen(server_id) + 1)) {
	    write_log("buf[%d]=%s\n", r, buf);
	    if (!strcmp(buf, server_id)) {
		return 1;
	    }
	}
    }

    return 0;
}

static int send_screen_update_header(struct projector_t *projector, int x, int y, int w, int h, int pixfmt, int len)
{
    req_screen_update data = {
	.x = htonl(x),
	.y = htonl(y),
	.width = htonl(w),
	.height = htonl(h),
	.depth = htonl(pixfmt),
	.size = htonl(len)
    };

    if (tcp_write_all(projector, (char *)&data, sizeof(data)) != sizeof(data)) {
	return 0;
    }

    return 1;
}

static unsigned char *get_raw_screen_data(struct projector_t *projector, int reg_x, int reg_y, int reg_w, int reg_h, long unsigned int *outlen)
{
    if (!outlen) {
	return NULL;
    }

    *outlen = reg_w * reg_h * 4;

    unsigned char *buffer = (unsigned char *) malloc(*outlen);

    if (!buffer) {
	return NULL;
    }

    uint32_t *b = (uint32_t *) buffer;
    uint32_t *c = (uint32_t *) projector->compare_buf;

    for (int y = 0; y < reg_h; y++) {
	for (int x = 0; x < reg_w; x++) {
	    b[y * reg_w + x] = c[(y + reg_y) * projector->scrWidth + (x + reg_x)];
	}
    }

    return buffer;
}

static int screen_diff(struct projector_t *projector, void *framebuffer, int reg_x, int reg_y, int reg_w, int reg_h, struct region_data *region)
{
    int x, y;
    int xstep = 4 / projector->scrDepth;

    uint32_t *f, *c;

    projector->varblock.min_x = projector->varblock.min_y = 9999;
    projector->varblock.max_x = projector->varblock.max_y = -1;

    if (!projector->compare_buf) {
	projector->compare_buf = (char *)malloc(projector->scrWidth * projector->scrHeight * projector->scrDepth);
	memset(projector->compare_buf, 0, projector->scrWidth * projector->scrHeight * projector->scrDepth);
    }

    if (reg_x + reg_w > projector->scrWidth) {
	reg_w = projector->scrWidth - reg_x;
    }

    if (reg_y + reg_h > projector->scrHeight) {
	reg_h = projector->scrHeight - reg_y;
    }

    f = (uint32_t *) framebuffer;
    c = (uint32_t *) projector->compare_buf;

    for (y = reg_y; y < reg_y + reg_h; y++) {
	for (x = reg_x; x < reg_x + reg_w; x++) {
	    int offset = y * projector->scrWidth + x;

	    uint32_t pixel = f[offset];

	    if (pixel != c[offset]) {
		c[offset] = pixel;

		if (x < projector->varblock.min_x) {
		    projector->varblock.min_x = x;
		}

		if (x > projector->varblock.max_x) {
		    projector->varblock.max_x = x;
		}

		if (y < projector->varblock.min_y) {
		    projector->varblock.min_y = y;
		}

		if (y > projector->varblock.max_y) {
		    projector->varblock.max_y = y;
		}
	    }
	}
    }

    if (projector->varblock.min_x < 9999) {
	if (projector->varblock.max_x < 0) {
	    projector->varblock.max_x = projector->varblock.min_x;
	}

	if (projector->varblock.max_y < 0) {
	    projector->varblock.max_y = projector->varblock.min_y;
	}

	if (projector->varblock.max_x - projector->varblock.min_x < 8) {
	    int need = 8 - (projector->varblock.max_x - projector->varblock.min_x);
	    if (projector->varblock.max_x < projector->scrWidth - 8) {
		projector->varblock.max_x += need;
	    } else {
		projector->varblock.min_x -= need;
	    }
	}

	if (projector->varblock.max_y - projector->varblock.min_y < 8) {
	    int need = 8 - (projector->varblock.max_y - projector->varblock.min_y);
	    if (projector->varblock.max_y < projector->scrHeight - 8) {
		projector->varblock.max_y += need;
	    } else {
		projector->varblock.min_y -= need;
	    }
	}

	if (projector->varblock.max_y >= projector->scrHeight) {
	    projector->varblock.max_y = projector->scrHeight - 1;
	}

//#ifdef DEBUG_SCREEN
//	write_log("Modified rect %d x %d - %d x %d\n",
//		projector->varblock.min_x, projector->varblock.min_y, projector->varblock.max_x, projector->varblock.max_y);
//#endif

	int rect_width = projector->varblock.max_x - projector->varblock.min_x + 1;
	int rect_height = projector->varblock.max_y - projector->varblock.min_y + 1;

	unsigned char *outbuffer = NULL;
	long unsigned int outlen;
	compress_rgb_to_jpeg(framebuffer, projector->varblock.min_x, projector->varblock.min_y, rect_width, rect_height,
			     projector->scrWidth * projector->scrDepth, projector->scrDepth * 8, &outbuffer, &outlen, 75);

	uint32_t pixfmt = PIX_JPEG_BGRA;

	if (outlen > rect_width * rect_height * 4) {
	    free(outbuffer);

//	    write_log("compressed size (%d) > uncompressed(%d - [%d, %d])\n", outlen, rect_width * rect_height * 4, rect_width, rect_height);
	    outbuffer = get_raw_screen_data(projector, projector->varblock.min_x, projector->varblock.min_y, rect_width, rect_height, &outlen);
	    pixfmt = PIX_RAW_BGRA;

	    char *tmpbuf = (char *) malloc(outlen);
	    int tmplen = LZ4_compress_default((const char *)outbuffer, tmpbuf, outlen, outlen);

	    if (tmplen > 0 && tmplen < outlen) {
		free(outbuffer);
		outbuffer = (unsigned char *)tmpbuf;
		outlen = tmplen;
		pixfmt = PIX_LZ4_BGRA;
	    } else {
		free(tmpbuf);
	    }
	}

	region->x = projector->varblock.min_x;
	region->y = projector->varblock.min_y;
	region->width = rect_width;
	region->height = rect_height;
	region->pixfmt = pixfmt;
	region->length = outlen;
	region->data = outbuffer;

	return 1;

//	write_log("Update header\n");
//	send_screen_update_header(projector,
//				  projector->varblock.min_x,
//				  projector->varblock.min_y, rect_width, rect_height, pixfmt, outlen);

//	write_log("send screen %d - %02X %02X %02X %02X\n", outlen, outbuffer[0], outbuffer[1], outbuffer[2], outbuffer[3]);

//	if (tcp_write_all(projector, (char *)outbuffer, outlen) != outlen) {
//	    write_log("Error sending framebuffer!\n");
//	}

#if PICS_DUMP
	static int fnamecount = 0;
	char fname[256];
	snprintf(fname, sizeof(fname), "/tmp/pic-%d.jpg", fnamecount++);
	dump_file(fname, (char *)outbuffer, outlen);
#endif

//	free(outbuffer);
    } else {
	//write_log("Update header\n");
//	send_screen_update_header(projector, 0, 0, 0, 0, 0, 0);
	//write_log("send screen %d\n", 0);
	return 0;
    }
}

static void update_screen(void *arg, void *fb)
{
    struct projector_t *projector = (struct projector_t *)arg;

    struct region_data region[projector->blocks_x * projector->blocks_y];

    int region_count = 0;

    for (int y = 0; y < projector->blocks_y; y++) {
	for (int x = 0; x < projector->blocks_x; x++) {
	    if (screen_diff(projector, fb, x * BLOCK_WIDTH, y * BLOCK_HEIGHT, BLOCK_WIDTH, BLOCK_HEIGHT, &region[region_count])) {
		region_count++;
	    }
	}
    }

    req_screen_regions req = {
	.req = htonl(REQ_SCREEN_UPDATE),
	.regions = htonl(region_count)
    };

    if (tcp_write_all(projector, (char *)&req, sizeof(req)) != sizeof(req)) {
	return;
    }

    int i = 0;
    while (region_count--) {
	send_screen_update_header(projector, region[i].x, region[i].y, region[i].width, region[i].height, region[i].pixfmt, region[i].length);

	if (tcp_write_all(projector, (char *)region[i].data, region[i].length) != region[i].length) {
	    write_log("Error sending framebuffer!\n");
	}
	free(region[i].data);
	i++;
    }
}

static void finish_fb(struct projector_t *projector)
{
    GrabberFinish(projector->xgrabber);
    if (projector->compare_buf) {
	free(projector->compare_buf);
    }
}

static int send_req_screen_info(struct projector_t *projector)
{
    req_screen_info data = {
	.req = htonl(REQ_SCREEN_INFO),
	.width = htonl(projector->scrWidth),
	.height = htonl(projector->scrHeight),
	.depth = htonl(projector->scrDepth * 8),
	.pixelformat = htonl(0)
    };

    if (tcp_write_all(projector, (char *)&data, sizeof(data)) != sizeof(data)) {
	return 0;
    }

    return 1;
}

static int send_req_screen_update(struct projector_t *projector)
{
    GrabberGetScreen(projector->xgrabber, 0, 0, projector->scrWidth, projector->scrHeight, update_screen, projector);

    return 1;
}

static int send_req_appserver_host(struct projector_t *projector, const char *apphost)
{
    int len = apphost ? strlen(apphost) : 0;

    req_appserver_host data = {
	.req = htonl(REQ_APPSERVER_HOST),
	.length = htonl(len)
    };

    if (tcp_write_all(projector, (char *)&data, sizeof(data)) != sizeof(data)) {
	return 0;
    }

    if (tcp_write_all(projector, (char *)apphost, len) != len) {
	return 0;
    }

    return 1;
}

static int send_req_session_id(struct projector_t *projector, const char *session_id)
{
    int len = session_id ? strlen(session_id) : 0;

    req_session_id data = {
	.req = htonl(REQ_SESSION_ID),
	.length = htonl(len)
    };

    if (tcp_write_all(projector, (char *)&data, sizeof(data)) != sizeof(data)) {
	return 0;
    }

    if (tcp_write_all(projector, (char *)session_id, len) != len) {
	return 0;
    }

    return 1;
}

static int send_req_hostname(struct projector_t *projector)
{
    char host[256];

    if (gethostname(host, sizeof(host)) != 0) {
	strcpy(host, "Unknown");
    }

    int len = strlen(host);

    req_hostname data = {
	.req = htonl(REQ_HOSTNAME),
	.length = htonl(len)
    };

    if (tcp_write_all(projector, (char *)&data, sizeof(data)) != sizeof(data)) {
	return 0;
    }

    if (tcp_write_all(projector, host, len) != len) {
	return 0;
    }

    return 1;
}

static int recv_authorization(struct projector_t *projector, char *buf, int len)
{
    uint32_t length = 0;

    if (!recv_uint32(projector, &length)) {
	return 0;
    }

    len--;

    length = (length < len) ? length : len;

    if (tcp_read_all(projector, buf, length) != length) {
	return 0;
    }

    buf[length] = 0;

    return 1;
}

static int recv_user_password(struct projector_t *projector)
{
    uint32_t length = 0;

    if (!recv_uint32(projector, &length)) {
	return 0;
    }

    projector->user_password = (char *)malloc(length + 1);
    if (tcp_read_all(projector, projector->user_password, length) != length) {
	free(projector->user_password);
	projector->user_password = NULL;
	return 0;
    }

    projector->user_password[length] = 0;

    return 1;
}

static int recv_user_connection(struct projector_t *projector)
{
    uint32_t tmp = 0;

    if (!recv_uint32(projector, &tmp)) {
	return 0;
    }

    projector->user_port = tmp;

    if (!recv_uint32(projector, &tmp)) {
	return 0;
    }

    projector->user_ipv6port = tmp;

    return 1;
}

static int recv_keyboard(struct projector_t *projector)
{
    uint32_t down, key;

    if (!recv_uint32(projector, &down)) {
	return 0;
    }

    if (!recv_uint32(projector, &key)) {
	return 0;
    }

    GrabberKeyboardEvent(projector->xgrabber, down, key);

    return 1;
}

static int recv_pointer(struct projector_t *projector)
{
    uint32_t buttons, x, y, wheel;

    if (!recv_uint32(projector, &buttons)) {
	return 0;
    }

    if (!recv_uint32(projector, &x)) {
	return 0;
    }

    if (!recv_uint32(projector, &y)) {
	return 0;
    }

    if (!recv_uint32(projector, &wheel)) {
	return 0;
    }

    GrabberMouseEvent(projector->xgrabber, buttons, x, y);

    return 1;
}

struct projector_t *projector_init()
{
    signal(SIGPIPE, SIG_IGN);

    write_log("Initializing screen device ...\n");
    return init_fb();
}

static int check_ssl_certificate(struct projector_t *projector)
{
    int err;
    write_log("Check remote SSL certificate\n");
    if((err = SSL_get_verify_result(projector->channel->ssl)) != X509_V_OK) {
	char buf[2048];
	X509 *cert = SSL_get_peer_certificate(projector->channel->ssl);
	if (cert) {
	    char *line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
	    char *line2 = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
	    snprintf(buf, sizeof(buf), "Subject: %s\nIssuer: %s\n", line, line2);
	    free(line);
	    free(line2);
	    X509_free(cert);
	    write_log("Cerificate:\n%s\n", buf);
	    if (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT || err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) {
		return 1;
	    }
	} else {
	    strncpy(buf, "Can't get remote certificate!\n", sizeof(buf));
	}
	write_log("Certificate verification error: %ld\n", err);
	int ret = projector->cb_ssl_verify(err, buf);
	if (ret) {
	    return 1;
	}
	return 0;
    }

    return 1;
}

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
	    write_log("%s: port error '%s'", __func__, ptr);
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

static char *header_get_path(char *header)
{
    char *tmp = strchr(header, ' ');
    if (tmp) {
	char *tmp1 = strchr(tmp + 1, ' ');
	if (tmp1) {
	    return strndup(tmp + 1, tmp1 - tmp - 1);
	}
    }
    return NULL;
}

int projector_connect(struct projector_t *projector, const char *controlhost, const char *apphost, const char *privkey, const char *cert, const char *session_id, int *is_started)
{
    int status = STATUS_OK;
    char *scheme = NULL;
    char *host = NULL;
    uint16_t port = 0;
    char *path = NULL;

    if (parse_url((char *)controlhost, &scheme, &host, &port, &path)) {
	write_log("Control host url parse error!\n");
	status =  STATUS_URL_PARSE_ERROR;
	goto err;
    }

    write_log("Connection:\n scheme %s\n host %s\n port %d\n path %s\n", scheme, host, port, path);

    int connect_method = -1;
    int connect_type = -1;

    if (scheme && (!strcmp(scheme, "http://") || !strcmp(scheme, "ws://") || !strcmp(scheme, "tcp://"))) {
	if (apphost) {
	    connect_type = TCP_CLIENT;
	} else {
	    connect_type = TCP_SERVER;
	}
	if (!strcmp(scheme, "ws://") || !strcmp(scheme, "http://")) {
	    connect_method = CONNECTION_METHOD_WS;
	} else {
	    connect_method = CONNECTION_METHOD_DIRECT;
	}
    } else if (scheme && (!strcmp(scheme, "https://") || !strcmp(scheme, "wss://") || !strcmp(scheme, "tcp+ssl://"))) {
	if (apphost) {
	    connect_type = TCP_SSL_CLIENT;
	} else {
	    connect_type = TCP_SSL_SERVER;
	}
	if (!strcmp(scheme, "wss://") || !strcmp(scheme, "https://")) {
	    connect_method = CONNECTION_METHOD_WS;
	} else {
	    connect_method = CONNECTION_METHOD_DIRECT;
	}
    }

    if (connect_type == -1) {
	write_log("Unknown control server url scheme %s!\n", scheme);
	status = STATUS_URL_SCHEME_UNKNOWN;
	goto err;
    }

    if (port == 0) {
	write_log("Control server port %d error!\n", port);
	goto err;
    }

    tcp_channel *server = NULL;

    if (connect_type == TCP_SERVER || connect_type == TCP_SSL_SERVER) {
	server = tcp_open(connect_type, host, port, (char *)privkey, (char *)cert);
    } else {
	projector->channel = tcp_open(connect_type, host, port, NULL, NULL);
    }

    do {

    if (connect_type == TCP_SERVER || connect_type == TCP_SSL_SERVER) {
	while (*is_started) {
	    projector->channel = tcp_accept(server);
	    if (!projector->channel) {
		fprintf(stderr, "tcp_accept()\n");
		continue;
	    }
	    break;
	}

	char request[1024];
	if ((connect_method == CONNECTION_METHOD_WS) &&
	    (!tcp_connection_upgrade(projector->channel, SIMPLE_CONNECTION_METHOD_WS, path, request, sizeof(request)))) {
	    write_log("http request %s\n", request);

	    char *file_path = header_get_path(request);
	    if (!strncmp(request, "GET ", 4) && !strncmp(path, file_path, strlen(path))) {
		write_log("requested path %s\n", file_path);
		int len;
		const char *mime;
		const char *file;
		char *tmp = file_path + strlen(path) + 1;
		if (*tmp == '/' || *tmp == 0) {
		    tmp = "index.html";
		}
		if ((file = pseudofs_get_file(tmp, &len, &mime))) {
		    write_log("found file %s len %d\n", file_path + strlen(path), len);
		    snprintf(request, sizeof(request), "HTTP/1.1 200 OK\r\nContent-type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", mime, len);
		    if (tcp_write(projector->channel, request, strlen(request)) == strlen(request)) {
			if (tcp_write(projector->channel, (char *)file, len) != len) {
			    write_log("Can't send http page body\n");
			}
		    } else {
			    write_log("Can't send http page header\n");
		    }
		} else {
		    snprintf(request, sizeof(request), "HTTP/1.1 404 Not Found\r\n\r\n");
		    tcp_write(projector->channel, request, strlen(request));
		}
		tcp_close(projector->channel);
		free(file_path);
		continue;
	    }

	    free(file_path);
	    write_log("%s: http ws method error!\n", __func__);
	    status = STATUS_CONNECTION_REJECTED;
	    *is_started = 0;
	}

	if (!*is_started) {
	    break;
	}
    }

    if (projector->channel && check_ssl_certificate(projector)) {
	if (check_remote_sig(projector, (connect_type == TCP_SERVER || connect_type == TCP_SSL_SERVER))) {
	    int authorized = !(connect_type == TCP_SERVER || connect_type == TCP_SSL_SERVER);
	    write_log("Start screen sharing\n");
	    projector->cb_error("Online");
	    while (*is_started) {
		uint32_t req;
//		write_log("Waiting request\n");
		if (recv_uint32(projector, &req)) {
//		    write_log("Received request %d\n", req);
		    if (!authorized && req != REQ_AUTHORIZATION) {
			    write_log("Not authorized!\n");
			    status = STATUS_NOT_AUTHORIZED;
			    break;
		    } else if (req == REQ_AUTHORIZATION) {
			char client_session_id[256];
			if (!recv_authorization(projector, client_session_id, sizeof(client_session_id))) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
			if (strcmp(session_id, client_session_id)) {
			    write_log("Wrong password [%s]!\n", client_session_id);
			    status = STATUS_NOT_AUTHORIZED;
			    send_uint32(projector, 1);
			    break;
			}
			if (!send_uint32(projector, 0)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
			authorized = 1;
		    } else if (req == REQ_SCREEN_INFO) {
			if (!send_req_screen_info(projector)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
		    } else if (req == REQ_APPSERVER_HOST) {
			if (!send_req_appserver_host(projector, apphost)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
		    } else if (req == REQ_SESSION_ID) {
			if (!send_req_session_id(projector, session_id)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
		    } else if (req == REQ_HOSTNAME) {
			if (!send_req_hostname(projector)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
		    } else if (req == REQ_SCREEN_UPDATE) {
			if (!send_req_screen_update(projector)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
		    } else if (req == REQ_USER_PASSWORD) {
			if (!recv_user_password(projector)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
			projector->cb_user_password(projector->user_password);
			write_log("User password is %s\n", projector->user_password);
		    } else if (req == REQ_USER_CONNECTION) {
			if (!recv_user_connection(projector)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
			projector->cb_user_connection(projector->user_port, projector->user_ipv6port);
			write_log("User connection ipv4 port is %d, ipv6 port is %d\n", projector->user_port, projector->user_ipv6port);
		    } else if (req == REQ_KEYBOARD) {
			if (!recv_keyboard(projector)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
		    } else if (req == REQ_POINTER) {
			if (!recv_pointer(projector)) {
			    status = STATUS_CONNECTION_ERROR;
			    break;
			}
		    } else if (req == REQ_STOP) {
			break;
		    } else {
			status = STATUS_UNSUPPORTED_REQUEST;
		    }
		} else {
		    status = STATUS_CONNECTION_ERROR;
		    break;
		}
	    }
	    if (projector->user_password) {
		free(projector->user_password);
	    }
	    write_log("Stop screen sharing\n");
	    projector->cb_error("Offline");
	} else {
	    write_log("Wrong server signature!\n");
	    status = STATUS_VERSION_ERROR;
	    *is_started = 0;
	    projector->cb_error("Error!");
	}
	tcp_close(projector->channel);
    } else {
	write_log("Connection error!\n");
	status = STATUS_CONNECTION_ERROR;
	projector->cb_error("Conn Error!");
    }

    } while ((connect_type == TCP_SERVER || connect_type == TCP_SSL_SERVER) && *is_started);

    if (server) {
	tcp_close(server);
    }

err:
    if (scheme) {
	free(scheme);
    }
    if (host) {
	free(host);
    }
    if (path) {
	free(path);
    }

    return status;
}

void projector_finish(struct projector_t *projector)
{
    write_log("Cleaning up...\n");
    finish_fb(projector);
    free(projector);
}
