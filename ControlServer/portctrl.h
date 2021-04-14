#ifndef __PORT_H__
#define __PORT_H__

int portctrl_init(unsigned int base, unsigned int num);

unsigned int portctrl_alloc();

int portctrl_free(unsigned int num);

void portctrl_finish();

#endif
