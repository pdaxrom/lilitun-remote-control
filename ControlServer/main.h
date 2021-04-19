#ifndef __MAIN_H__
#define __MAIN_H__

struct remote_connection_t;

#include <rfb/rfb.h>
#include <rfb/keysym.h>
#include <simple-connection/tcp.h>
#include "../Common/protos.h"
#include "list.h"
#include "jpegdec.h"
#include "fbops.h"
#include "portctrl.h"
#include "translator.h"
#include "getrandom.h"
#include "messaging.h"
#include "utils.h"
#include "../thirdparty/lz4/lz4.h"
#include "config.h"

struct control_t {
    char *sslcertfile;
    char *sslkeyfile;
    int use_ssl;
    int port;
};

struct control_connection_t {
    tcp_channel *channel;
};

struct screen_info_t {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t pixelformat;
};

struct remote_connection_t {
    tcp_channel *channel;
    struct screen_info_t screen_info;
    uint8_t *framebuffer;
    pthread_mutex_t framebuffer_mutex;
    uint32_t input_events[1024];
    int input_events_counter;
    req_update_pointer pointer_old;
    pthread_mutex_t projector_io_mutex;
    char *host;
    char *httpdir;
    char *sslkeyfile;
    char *sslcertfile;
    int use_ssl;
    int client_use_ssl;
    char **passwds;
    int httpport;
    int http6port;
    int port;
    int ipv6port;
    char *apphost;
    char *session_id;
    char *hostname;
    int thread_alive;
    int stop_req;
    struct translator_t *translator;
    pthread_t tid;
};

#endif
