#ifndef __TRANSLATOR_H__
#define __TRANSLATOR_H__

//struct _rfbClientRec;

struct translator_t *translator_init(struct remote_connection_t *conn,
				     void (*cb_keyboard)(signed char down, unsigned int key, struct _rfbClientRec *),
				     void(*cb_pointer)(int buttons, int x, int y, struct _rfbClientRec *));

void translator_finish(struct translator_t *translator);

#endif
