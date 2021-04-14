/**
 * @file config.h
 * @brief Configuration file parser
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

enum {
    CONF_ERROR = -1,
    CONF_OK = 0
};

typedef struct {
    FILE *inf;
    int secPos;
} Config;

#ifdef	__cplusplus
extern "C" {
#endif

Config *ConfigOpen(char *file);
int     ConfigSection(Config *conf, char *section);
char   *ConfigReadString(Config *conf, char *key, char *val, int len, char *defval);
int     ConfigReadInt(Config *conf, char *key, int defval);
void    ConfigClose(Config *conf);

char   *ConfigGetStringData(char *in, char *configPath, int freeIn);

#ifdef	__cplusplus
}
#endif

#endif
