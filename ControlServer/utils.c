#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "getrandom.h"

#define NIB2HEX(b)	(((b) < 10) ? ('0' + (b)) : ('a' + (b) - 10))
#define HEX2NIB(h)	(((h) >= '0' && (h) <= '9') ? ((h) - '0') : ((h) >= 'A' && (h) <= 'F') ? ((h) - 'A' + 10) : ((h) >= 'a' && (h) <= 'f') ? ((h) - 'a' + 10) : 0)

void byte_to_hex(uint8_t * buf, size_t n, char *hex)
{
    int i;
    for (i = 0; i < n; i++) {
	hex[i * 2 + 0] = NIB2HEX(buf[i] >> 4);
	hex[i * 2 + 1] = NIB2HEX(buf[i] & 0x0f);
    }
}

void hex_to_byte(char *hex, size_t n, uint8_t * buf)
{
    int i;
    for (i = 0; i < n; i++) {
	buf[i] = (HEX2NIB((uint8_t) hex[i * 2 + 0]) << 4) | HEX2NIB((uint8_t) hex[i * 2 + 1]);
    }
}

char *get_random_string(size_t len)
{
    uint8_t in[len];
    char *out;
    
    if (get_random(in, len, 0) == -1) {
	fprintf(stderr, "%s: get_random()\n", __func__);
	return NULL;
    }

    out = (char *) malloc(len * 2 + 1);
    out[len * 2] = 0;

    byte_to_hex(in, len, out);

    return out;
}

char *copy_string(char *dst, int dstSize, char *src, int srcSize)
{
    if (!dst) {
	dstSize = srcSize;
	dst = malloc(dstSize + 1);
    } else {
	dstSize = (srcSize > dstSize) ? dstSize : srcSize;
    }
    memcpy(dst, src, dstSize);
    dst[dstSize] = 0;
    return dst;
}

char *request_parse(char *url, char *param, char *val, int n)
{
    char *ptr = strchr(url, '?');
    if (!ptr) {
	return NULL;
    }

    ptr++;
    while (ptr) {
	if (!strncmp(ptr, param, strlen(param))) {
	    ptr = ptr + strlen(param);
	    if (*ptr == '=') {
		char *ptr2 = ++ptr;
		while ((*ptr2 != '&') && (*ptr2 > ' ')) {
		    ptr2++;
		}
		return copy_string(val, n, ptr, ptr2 - ptr);
	    }
	}
	ptr = strchr(ptr, '&');
	if (ptr) {
	    ptr++;
	}
    }

    return NULL;
}

int select_read_timeout(int fd, int timeout)
{
    fd_set rfds;
    struct timeval tv;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    return select(fd + 1, &rfds, NULL, NULL, &tv);
}
