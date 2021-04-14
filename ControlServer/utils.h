#ifndef __UTILS_H__
#define __UTILS_H__

void byte_to_hex(uint8_t * buf, size_t n, char *hex);

void hex_to_byte(char *hex, size_t n, uint8_t * buf);

char *get_random_string(size_t len);

char *copy_string(char *dst, int dstSize, char *src, int srcSize);

char *request_parse(char *url, char *param, char *val, int n);

int select_read_timeout(int fd, int timeout);

#endif
