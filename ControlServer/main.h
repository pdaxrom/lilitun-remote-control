#ifndef __MAIN_H__
#define __MAIN_H__

struct remote_connection_t;

#include <simple-connection/tcp.h>
#include "../Common/protos.h"
#include "list.h"
#include "getrandom.h"
#include "messaging.h"
#include "utils.h"
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

struct remote_connection_t {
    int type;
    pthread_mutex_t projector_io_mutex;
    tcp_channel *channel;
    char *host;
    char *httpdir;
    char *sslkeyfile;
    char *sslcertfile;
    int use_ssl;
    int client_use_ssl;
    char *apphost;
    char *session_id;
    char *hostname;
    int thread_alive;
    int stop_req;
    pthread_t tid;
};

#endif
