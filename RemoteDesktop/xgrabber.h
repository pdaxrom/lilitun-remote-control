#ifndef __XCBGRABBER_H__
#define __XCBGRABBER_H__

struct XGrabber;

#ifdef __cplusplus
extern "C" {
#endif

struct XGrabber *GrabberInit(int *w, int *h, int *d);
void GrabberFinish(struct XGrabber *cfg);
int GrabberGetScreen(struct XGrabber *cfg, int x, int y, int w, int h, void (*cb)(void *arg, void *fb), void *arg);
void GrabberMouseEvent(struct XGrabber *cfg, uint32_t buttons, int x, int y);
void GrabberKeyboardEvent(struct XGrabber *cfg, int down, uint32_t key);

#ifdef __cplusplus
}
#endif

#endif
