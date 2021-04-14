#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <xcb/xcb.h>
#include <sys/shm.h>
#include <xcb/shm.h>
#include <xcb/xtest.h>
#include <xcb/xcb_keysyms.h>
#include <inttypes.h>

#define XCB_ALL_PLANES (~0)

typedef struct {
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    uint32_t shmid;
    uint8_t *shmaddr;
    xcb_shm_seg_t shmseg;

    xcb_key_symbols_t *syms;
    uint32_t buttons;
} XGrabber;

static int CheckXcbShm(XGrabber * cfg)
{
    xcb_shm_query_version_cookie_t cookie = xcb_shm_query_version(cfg->connection);
    xcb_shm_query_version_reply_t *reply;

    reply = xcb_shm_query_version_reply(cfg->connection, cookie, NULL);
    if (reply) {
	free(reply);
	return 1;
    }

    return 0;
}

static int AllocateShmBuffer(XGrabber * cfg, int max_size)
{
    cfg->shmid = shmget(IPC_PRIVATE, max_size, IPC_CREAT | 0777);
    if (cfg->shmid < 0) {
	fprintf(stderr, "shmget() error!\n");
	return 0;
    }

    cfg->shmseg = xcb_generate_id(cfg->connection);
    xcb_shm_attach(cfg->connection, cfg->shmseg, cfg->shmid, 0);
    cfg->shmaddr = shmat(cfg->shmid, NULL, 0);
    shmctl(cfg->shmid, IPC_RMID, 0);
    if ((int64_t) cfg->shmaddr == -1 || !cfg->shmaddr) {
	fprintf(stderr, "shmat() error!\n");
	return 0;
    }

    return 1;
}

void FreeShmBuffer(XGrabber * cfg)
{
    xcb_shm_detach_checked(cfg->connection, cfg->shmseg);

    shmdt(cfg->shmaddr);
}

int GrabberGetScreen(XGrabber * cfg, int x, int y, int w, int h, void (*cb)(void *arg, void *fb), void *arg)
{
    xcb_generic_error_t *e = NULL;
    xcb_shm_get_image_cookie_t cookie =
	xcb_shm_get_image(cfg->connection, cfg->screen->root, x, y, w, h, XCB_ALL_PLANES, XCB_IMAGE_FORMAT_Z_PIXMAP,
			  cfg->shmseg, 0);
    xcb_shm_get_image_reply_t *reply = xcb_shm_get_image_reply(cfg->connection, cookie, &e);
    xcb_flush(cfg->connection);
    if (e) {
	fprintf(stderr,
		"Cannot get the image data "
		"event_error: response_type:%u error_code:%u "
		"sequence:%u resource_id:%u minor_code:%u major_code:%u.\n",
		e->response_type, e->error_code, e->sequence, e->resource_id, e->minor_code, e->major_code);
	free(e);
    }

    if (reply != NULL) {
	if (cb) {
	    cb(arg, cfg->shmaddr);
	}

	free(reply);
	return 1;
    }
    return 0;
}

static xcb_screen_t *ScreenOfDisplay(XGrabber * cfg, int screenNum)
{
    xcb_screen_iterator_t iter;

    iter = xcb_setup_roots_iterator(xcb_get_setup(cfg->connection));
    for (; iter.rem; --screenNum, xcb_screen_next(&iter)) {
	if (screenNum == 0) {
	    cfg->screen = iter.data;
	    return cfg->screen;
	}
    }

    return NULL;
}

XGrabber *GrabberInit(int *w, int *h, int *d)
{
    int screenNum;
    XGrabber *cfg = malloc(sizeof(XGrabber));

    cfg->buttons = 0;

    cfg->connection = xcb_connect(NULL, &screenNum);

    if (!CheckXcbShm(cfg)) {
	fprintf(stderr, "no xcb shm!\n");
	xcb_disconnect(cfg->connection);
	free(cfg);
	return NULL;
    }

    ScreenOfDisplay(cfg, screenNum);

    fprintf(stderr, "\n");
    fprintf(stderr, "Informations of screen %" PRIu32 ":\n", cfg->screen->root);
    fprintf(stderr, "  width.........: %" PRIu16 "\n", cfg->screen->width_in_pixels);
    fprintf(stderr, "  height........: %" PRIu16 "\n", cfg->screen->height_in_pixels);
    fprintf(stderr, "  white pixel...: %" PRIu32 "\n", cfg->screen->white_pixel);
    fprintf(stderr, "  black pixel...: %" PRIu32 "\n", cfg->screen->black_pixel);
    fprintf(stderr, "\n");

    *w = cfg->screen->width_in_pixels;
    *h = cfg->screen->height_in_pixels;
    *d = cfg->screen->root_depth / 8;
    if (*d == 3) {
	*d = 4;
    }

    if (!AllocateShmBuffer(cfg, (*w) * (*h) * (*d))) {
	fprintf(stderr, "Cannot allocate shm buffer!\n");
	xcb_disconnect(cfg->connection);
	free(cfg);
	return NULL;
    }

    cfg->syms = xcb_key_symbols_alloc(cfg->connection);

    return cfg;
}

void GrabberFinish(XGrabber * cfg)
{
    FreeShmBuffer(cfg);
    xcb_key_symbols_free(cfg->syms);
    xcb_disconnect(cfg->connection);
    free(cfg);
}

static void setButtons(XGrabber * cfg, uint32_t buttons, uint32_t mask, int button)
{
    if ((cfg->buttons & mask) != (buttons & mask)) {
	if (cfg->buttons & mask) {
	    cfg->buttons &= ~mask;
	    xcb_test_fake_input(cfg->connection, XCB_BUTTON_RELEASE, button, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
//          fprintf(stderr, "Release button %d %d\n", mask, button);
	} else {
	    cfg->buttons |= mask;
	    xcb_test_fake_input(cfg->connection, XCB_BUTTON_PRESS, button, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
//          fprintf(stderr, "Press button %d %d\n", mask, button);
	}
    }
}

void GrabberMouseEvent(XGrabber * cfg, uint32_t buttons, int x, int y)
{
//    xcb_warp_pointer(cfg->connection, XCB_NONE, cfg->screen->root, 0, 0, 0, 0, x, y);
    xcb_test_fake_input(cfg->connection, XCB_MOTION_NOTIFY, 0, XCB_CURRENT_TIME, cfg->screen->root, x, y, 0);
    setButtons(cfg, buttons, 1, 1);
    setButtons(cfg, buttons, 2, 2);
    setButtons(cfg, buttons, 4, 3);
}

void GrabberKeyboardEvent(XGrabber * cfg, int down, uint32_t key)
{
    xcb_keycode_t *keycode = xcb_key_symbols_get_keycode(cfg->syms, key);

    if (keycode) {
	//fprintf(stderr, "Key [%02X]\n", keycode[0]);

	if (down) {
	    xcb_test_fake_input(cfg->connection, XCB_KEY_PRESS, keycode[0], XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
	} else {
	    xcb_test_fake_input(cfg->connection, XCB_KEY_RELEASE, keycode[0], XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
	}
    }
}
