/**
 * @file getrandom.h
 * @brief Working with random bytes function
 */
#ifndef __GETRANDOM_H__
#define __GETRANDOM_H__

int get_random(void *buf, size_t buflen, unsigned int flags);

#endif
