#include <stdio.h>
#ifdef __linux__
#include <unistd.h>
#define _GNU_SOURCE
#include <sys/syscall.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include "getrandom.h"

/**
 * @brief Get random bytes to buffer
 * @param buf Buffer
 * @param buflen Buffer length
 * @param flags Flags
 * @return Status
 */
int get_random(void *buf, size_t buflen, unsigned int flags)
{
#if defined(_WIN32)
    int ret = -1;
    HCRYPTPROV hProvider = 0;
    if (!CryptAcquireContextW(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
	return -1;
    }
    if (CryptGenRandom(hProvider, buflen, buf)) {
	ret = buflen;
    }
    CryptReleaseContext(hProvider, 0);
    return ret;
#else
    int ret;
    FILE *inf;
#if defined(__linux__)

#ifndef SYS_getrandom
#warning Trying to define SYS_getrandom
#if defined(__x86_64__)
#define SYS_getrandom 318
#elif defined(__i386__)
#define SYS_getrandom 355
#elif defined(__aarch64__)
#define SYS_getrandom 278
#elif defined(__arm__)
#define SYS_getrandom 384
#elif defined(__mips__)
#define SYS_getrandom 278
#elif defined(__powerpc__)
#define SYS_getrandom 359
#else
#error Unsupported arch!
#endif
#endif

    ret = (int)syscall(SYS_getrandom, buf, buflen, flags);
    if (ret != -1) {
	return ret;
    }
#endif
    inf = fopen("/dev/urandom", "rb");
    if (!inf) {
	return -1;
    }
    ret = fread(buf, 1, buflen, inf);
    fclose(inf);
    if (ret != buflen) {
	ret = -1;
    }
    return ret;
#endif
}
