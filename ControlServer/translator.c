#include <stdio.h>

#include "main.h"

#define BITS_PER_SAMPLE		8
#define SAMPLES_PER_PIXEL	4

struct varblock_t {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
};

struct translator_t {
    struct remote_connection_t *conn;
    uint8_t *framebuffer_vnc;
    uint8_t *framebuffer_cmp;
    struct varblock_t varblock;
    rfbScreenInfoPtr server;
    pthread_t tid;
    int is_running;
};

static void update_screen(struct translator_t *translator)
{
    pthread_mutex_lock(&translator->conn->framebuffer_mutex);

    int x, y;
    int xstep = 4 / (translator->conn->screen_info.depth / 8);

    uint32_t *f, *c, *r;

    translator->varblock.min_x = translator->varblock.min_y = 9999;
    translator->varblock.max_x = translator->varblock.max_y = -1;

    f = (uint32_t *) translator->conn->framebuffer;
    c = (uint32_t *) translator->framebuffer_cmp;
    r = (uint32_t *) translator->framebuffer_vnc;

    for (y = 0; y < translator->conn->screen_info.height; y++) {
	for (x = 0; x < translator->conn->screen_info.width; x++) {
	    uint32_t pixel = *f;

	    if (pixel != *c) {
		*c = pixel;
		*r = pixel;

		if (x < translator->varblock.min_x) {
		    translator->varblock.min_x = x;
		}

		if (x > translator->varblock.max_x) {
		    translator->varblock.max_x = x;
		}

		if (y < translator->varblock.min_y) {
		    translator->varblock.min_y = y;
		}

		if (y > translator->varblock.max_y) {
		    translator->varblock.max_y = y;
		}
	    }

	    f++;
	    c++;
	    r++;
	}
    }

    if (translator->varblock.min_x < 9999) {
	if (translator->varblock.max_x < 0) {
	    translator->varblock.max_x = translator->varblock.min_x;
	}

	if (translator->varblock.max_y < 0) {
	    translator->varblock.max_y = translator->varblock.min_y;
	}

	rfbMarkRectAsModified(translator->server, translator->varblock.min_x, translator->varblock.min_y,
			      translator->varblock.max_x + 2, translator->varblock.max_y + 1);

	rfbProcessEvents(translator->server, 10000);
    }

    pthread_mutex_unlock(&translator->conn->framebuffer_mutex);
}

static enum rfbNewClientAction new_client_hook(rfbClientPtr cl)
{
    struct remote_connection_t *conn = cl->screen->screenData;

    if (!cl->sslctx && conn->client_use_ssl) {
	fprintf(stderr, "Not ssl connection, reject.\n");
	return RFB_CLIENT_REFUSE;
    }
    return RFB_CLIENT_ACCEPT;
}

static void *server_thread(void *arg)
{
    struct translator_t *translator = (struct translator_t *)arg;

    while (translator->is_running) {
	while (translator->server->clientHead == NULL && translator->is_running) {
	    rfbProcessEvents(translator->server, 100000);
	}

	rfbProcessEvents(translator->server, 100000);

	update_screen(translator);
    }

    return NULL;
}

struct translator_t *translator_init(struct remote_connection_t *conn,
				     void (*cb_keyboard)(signed char down, unsigned int key, struct _rfbClientRec *),
				     void(*cb_pointer)(int buttons, int x, int y, struct _rfbClientRec *))
{
    int argc = 1;
    char *argv[] = { "Translator" };

    struct translator_t *translator = (struct translator_t *)malloc(sizeof(struct translator_t));
    translator->conn = conn;

    translator->framebuffer_vnc = (uint8_t *) malloc(conn->screen_info.width * conn->screen_info.height * (conn->screen_info.depth / 8));
    translator->framebuffer_cmp = (uint8_t *) malloc(conn->screen_info.width * conn->screen_info.height * (conn->screen_info.depth / 8));

    if (!translator->framebuffer_vnc || !translator->framebuffer_cmp) {
	fprintf(stderr, "ERROR: No memory for vnc framebuffers!\n");

	if (translator->framebuffer_vnc) {
	    free(translator->framebuffer_vnc);
	}

	if (translator->framebuffer_cmp) {
	    free(translator->framebuffer_cmp);
	}

	return NULL;
    }

    memset(translator->framebuffer_vnc, 0, conn->screen_info.width * conn->screen_info.height * (conn->screen_info.depth / 8));
    memset(translator->framebuffer_cmp, 0, conn->screen_info.width * conn->screen_info.height * (conn->screen_info.depth / 8));

    translator->server = rfbGetScreen(&argc, argv, conn->screen_info.width, conn->screen_info.height, BITS_PER_SAMPLE, SAMPLES_PER_PIXEL, conn->screen_info.depth / 8);
    if (!translator->server) {
	fprintf(stderr, "ERROR: rfbGetScreen()");

	free(translator->framebuffer_vnc);
	free(translator->framebuffer_cmp);

	return NULL;
    }

    translator->server->desktopName = conn->hostname;
    translator->server->frameBuffer = (char *)translator->framebuffer_vnc;
    translator->server->alwaysShared = TRUE;
    translator->server->httpDir = conn->httpdir;
    translator->server->httpPort = conn->httpport;
    translator->server->http6Port = conn->http6port;
    translator->server->port = conn->port;
    translator->server->ipv6port = conn->ipv6port;
    translator->server->sslkeyfile = conn->sslkeyfile;
    translator->server->sslcertfile = conn->sslcertfile;

    translator->server->authPasswdData = (void*)conn->passwds;
    translator->server->passwordCheck = rfbCheckPasswordByList;

    translator->server->kbdAddEvent = cb_keyboard;
    translator->server->ptrAddEvent = cb_pointer;

    translator->server->newClientHook = new_client_hook;

    translator->server->screenData = conn;

    rfbInitServer(translator->server);

    /* Mark as dirty since we haven't sent any updates at all yet. */
    rfbMarkRectAsModified(translator->server, 0, 0, conn->screen_info.width, conn->screen_info.height);

    translator->is_running = 1;

    if (pthread_create(&translator->tid, NULL, &server_thread, (void *)translator) != 0) {
	fprintf(stderr, "%s pthread_create(remote_connection_thread)\n", __func__);
	translator_finish(translator);
	return NULL;
    }

    return translator;
}

void translator_finish(struct translator_t *translator)
{
    translator->is_running = 0;

    pthread_join(translator->tid, NULL);

    rfbShutdownServer(translator->server, TRUE);
    rfbScreenCleanup(translator->server);

    free(translator->framebuffer_cmp);
    free(translator->framebuffer_vnc);
    free(translator);
}
