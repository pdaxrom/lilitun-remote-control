#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <syslog.h>
#include <limits.h>
#include <sys/stat.h>

#include "config.h"

#define SKIP_SPACES(p) while (isblank(*p)) p++;

#define SKIP_END_SPACES(p) \
    while (strlen(p) > 0 && isspace(*(p + strlen(p) - 1))) { \
	*(p + strlen(p) - 1) = 0; \
    }

/**
 * @brief Open configuration
 * @param file Config file
 * @return Config
 */
Config *ConfigOpen(char *file)
{
    Config *conf = malloc(sizeof(Config));
    if (conf) {
	conf->inf = fopen(file, "rb");
	if (conf->inf) {
	    conf->secPos = 0;
	    return conf;
	}
	free(conf);
    }
    return NULL;
}

/**
 * @brief Move to specific section
 * @param Config
 * @param Section name
 * @return Status
 */
int ConfigSection(Config *conf, char *section)
{
    if (section) {
	char buffer[1024];
	fseek(conf->inf, 0, SEEK_SET);
	while (fgets(buffer, sizeof(buffer), conf->inf)) {
	    char *ptr = buffer;
	    SKIP_SPACES(ptr);
	    SKIP_END_SPACES(ptr);
	    if (strlen(ptr) > 1) {
		if (*ptr == '[' && *(ptr + strlen(ptr) - 1) == ']' && !strncmp(ptr + 1, section, strlen(section))) {
		    conf->secPos = ftell(conf->inf);
		    return 0;
		}
	    }
	}
    }

    return -1;
}

/**
 * @brief Read string value from config
 * @param file Config file
 * @param key Key string
 * @param val String buffer
 * @param len String buffer length
 * @return String
 */
char *ConfigReadString(Config *conf, char *key, char *val, int len, char *defval)
{
    char buffer[1024];

    fseek(conf->inf, conf->secPos, SEEK_SET);

    while(fgets(buffer, sizeof(buffer), conf->inf)) {
	char *ptr = buffer;
	char *ptrNext;
	SKIP_SPACES(ptr);
	SKIP_END_SPACES(ptr);
	if (*ptr == 0) {
	    continue;
	}
	if (*ptr == '[') {
	    break;
	}
	if (*ptr == ';' || *ptr == '#') {
	    continue;
	}
	ptrNext = ptr;
	while (isalnum(*ptrNext) || *ptrNext == '-' || *ptrNext == '_') ptrNext++;
	if (*ptrNext == 0) {
	    continue;
	}
	*ptrNext++ = 0;
	if (!strcmp(ptr, key)) {
	    SKIP_SPACES(ptrNext);
	    if (!val) {
		return strdup(ptrNext);
	    } else {
		strncpy(val, ptrNext, len);
		return val;
	    }
	}
    }

    if (!val) {
	return defval ? strdup(defval) : NULL;
    } else {
	if (defval) {
	    strncpy(val, defval, len);
	    return val;
	} else {
	    val[0] = 0;
	    return NULL;
	}
    }
}

/**
 * @brief Read int value from config
 * @param file Config file
 * @param key Key string
 * @param val Value
 * @return Value
 */
int ConfigReadInt(Config *conf, char *key, int defval)
{
    int val;
    char *str = ConfigReadString(conf, key, NULL, 0, NULL);
    if (!str) {
	return defval;
    }
    val = atoi(str);
    free(str);
    return val;
}

/**
 * @brief Close configuration
 * @param conf Config
 */
void ConfigClose(Config *conf)
{
    if (conf) {
	fclose(conf->inf);
	free(conf);
    }
}

/**
 * @brief If string is file, read string from file
 */
char *ConfigGetStringData(char *in, char *configPath, int freeIn)
{
    char fname[PATH_MAX];
    struct stat sbuf;
    int ret;
    FILE *f;
    char buf[1024];
    char *retval = NULL;

    if (!in) {
	return NULL;
    }

    snprintf(fname, sizeof(fname), "%s", in);
    if ((ret = stat(fname, &sbuf)) == -1) {
	snprintf(fname, sizeof(fname), "%s/%s", configPath, in);
	if ((ret = stat(fname, &sbuf)) == -1) {
	    retval = strdup(in);
	    if (freeIn) {
		free(in);
	    }
	    return retval;
	}
    }

    f = fopen(fname, "rb");
    if (f) {
	if (fgets(buf, sizeof(buf), f)) {
	    char *ptr = buf;
	    SKIP_SPACES(ptr);
	    SKIP_END_SPACES(ptr);

	    if (strlen(ptr) > 0) {
		retval = strdup(ptr);
	    }
	}
	fclose(f);
    }

    if (freeIn) {
	free(in);
    }
    return retval;
}
