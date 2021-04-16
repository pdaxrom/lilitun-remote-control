#ifndef __PROTOS_H__
#define __PROTOS_H__

#define CLIENT_ID	"Projector application 1.0"
#define SERVER_ID	"Screen sharing server 1.0"

enum {
    REQ_ERROR = 0,
    REQ_SCREEN_INFO,
    REQ_SCREEN_UPDATE,
    REQ_KEYBOARD,
    REQ_POINTER,
    REQ_USER_CONNECTION,
    REQ_USER_PASSWORD,
    REQ_APPSERVER_HOST,
    REQ_SESSION_ID,
    REQ_HOSTNAME,
    REQ_STOP
};

enum {
    PIX_RAW_RGBA = 0,
    PIX_RAW_BGRA,
    PIX_JPEG_RGBA,
    PIX_JPEG_BGRA,
    PIX_LZ4_RGBA,
    PIX_LZ4_BGRA
};

#if _WIN32
#pragma pack(push, 1)
#endif

typedef struct __attribute__ ((__packed__)) {
    uint32_t req;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t pixelformat;
} req_screen_info;

typedef struct __attribute__ ((__packed__)) {
    uint32_t req;
    int regions;
} req_screen_regions;

typedef struct __attribute__ ((__packed__)) {
    int x;
    int y;
    int width;
    int height;
    int depth;
    int size;
} req_screen_update;

typedef struct __attribute__ ((__packed__)) {
    uint32_t down;
    uint32_t scancode;
} req_update_keyboard;

typedef struct __attribute__ ((__packed__)) {
    uint32_t buttons;
    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t wheel;
} req_update_pointer;

typedef struct __attribute__ ((__packed__)) {
    uint32_t length;
} req_user_password;

typedef struct __attribute__ ((__packed__)) {
    uint32_t port;
    uint32_t ipv6port;
} req_user_connection;

typedef struct __attribute__ ((__packed__)) {
    uint32_t req;
    uint32_t length;
} req_appserver_host;

typedef struct __attribute__ ((__packed__)) {
    uint32_t req;
    uint32_t length;
} req_session_id;

typedef struct __attribute__ ((__packed__)) {
    uint32_t req;
    uint32_t length;
} req_hostname;

#if _WIN32
#pragma pack(pop)
#endif

#endif
