#ifndef __PROJECTOR_H__
#define __PROJECTOR_H__

enum {
    CONNECTION_METHOD_DIRECT = 0,
    CONNECTION_METHOD_CONNECT,
    CONNECTION_METHOD_WS
};

enum {
    STATUS_OK = 0,
    STATUS_VERSION_ERROR,
    STATUS_CONNECTION_REJECTED,
    STATUS_CONNECTION_ERROR,
    STATUS_UNSUPPORTED_REQUEST,
    STATUS_URL_PARSE_ERROR,
    STATUS_URL_SCHEME_UNKNOWN,
    STATUS_URL_PORT_ERROR,
    STATUS_NOT_AUTHORIZED
};

struct varblock_t {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
};

struct projector_t {
    void *xgrabber;
    int scrWidth;
    int scrHeight;
    int scrDepth;
    int blocks_x;
    int blocks_y;
    char *compare_buf;
    struct varblock_t varblock;
    tcp_channel *channel;
    int user_port;
    int user_ipv6port;
    char *user_password;
    struct list_item *threads_list;
    pthread_mutex_t threads_list_mutex;
    void (*cb_error)(char *str);
    void (*cb_user_password)(char *str);
    void (*cb_user_connection)(int port, int ipv6port);
    int (*cb_ssl_verify)(int err, char *cert);
};

#ifdef __cplusplus
extern "C" {
#endif

struct projector_t *projector_init();
int projector_connect(struct projector_t *projector, const char *controlhost, const char *apphost, const char *privkey, const char *cert, const char *session_id, int *is_started);
void projector_finish(struct projector_t *projector);

#ifdef __cplusplus
}
#endif
#endif
