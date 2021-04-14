#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "base64.h"
#include "../thirdparty/jsmn/jsmn.h"
#include "app.h"

static const char *x_scheme_handler = "lilink://";

#ifdef _WIN32
static char *strndup(const char *str, size_t n)
{
    size_t len;
    char *copy;

    for (len = 0; len < n && str[len]; len++)
	continue;

    if ((copy = (char *) malloc(len + 1)) == NULL)
	return (NULL);
    memcpy(copy, str, len);
    copy[len] = '\0';
    return (copy);
}
#endif

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
	strncmp(json + tok->start, s, tok->end - tok->start) == 0) {

	return 0;
    }

    return -1;
}

const char *json_requestType = NULL;
const char *json_appServerUrl = NULL;
const char *json_controlServerUrl = NULL;
int   json_controlServerPort = 9998;
const char *json_sessionId = NULL;

static int extract_json_params(char *instr)
{
    jsmn_parser p;
    jsmntok_t t[16];

    size_t outlen = 0;

    if (strchr(instr, '/')) {
	char *ptr = strchr(instr, '/');
	*ptr = 0;
    }

    char *outstr = (char *) base64_decode((unsigned char *)instr, strlen(instr), &outlen);

    if (!outstr) {
	write_log("Cannot decode base64 string [%s]\n", instr);
	return 0;
    }

    outstr[outlen] = 0;

    fprintf(stderr, "LINK DECEODED [%s]\n", outstr);

    jsmn_init(&p);
    int r = jsmn_parse(&p, outstr, strlen(outstr), t,
			    sizeof(t) / sizeof(t[0]));
    if (r < 0) {
	write_log("Failed to parse JSON: %d\n", r);
	free(outstr);
	return 0;
    }

    if (r < 1 || t[0].type != JSMN_OBJECT) {
	write_log("Object expected\n");
	free(outstr);
	return 0;
    }

    for (int i = 1; i < r; i++) {
	if (jsoneq(outstr, &t[i], "requestType") == 0) {
	    json_requestType = strndup(outstr + t[i + 1].start, t[i + 1].end - t[i + 1].start);
	    i++;
	} else if (jsoneq(outstr, &t[i], "appServerUrl") == 0) {
	    json_appServerUrl = strndup(outstr + t[i + 1].start, t[i + 1].end - t[i + 1].start);
	    i++;
	} else if (jsoneq(outstr, &t[i], "controlServerUrl") == 0) {
	    json_controlServerUrl = strndup(outstr + t[i + 1].start, t[i + 1].end - t[i + 1].start);
	    i++;
	} else if (jsoneq(outstr, &t[i], "sessionId") == 0) {
	    json_sessionId = strndup(outstr + t[i + 1].start, t[i + 1].end - t[i + 1].start);
	    i++;
	}
    }

    if (strchr(json_controlServerUrl, ':')) {
	char *endptr = (char *)json_controlServerUrl + strlen(json_controlServerUrl);
	char *ptr = strchr((char *)json_controlServerUrl, ':');
	*ptr++ = 0;
	json_controlServerPort = 9998;
	if (ptr) {
	    unsigned int port = strtol(ptr, &endptr, 10);
	    if (port == 0 || port >= USHRT_MAX || (errno == ERANGE && (port == USHRT_MAX || port == 0))
		    || (errno != 0 && port == 0)) {
		write_log("controlServer port error '%s'", ptr);
	    } else {
		json_controlServerPort = port;
	    }
	}
    }

    free(outstr);

    return 1;
}

#ifdef _WIN32
static int register_uri_scheme(char *app)
{
	int ret = 0;
	HKEY hKey;
	LONG lResult;

	lResult = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\lilink", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
	if (lResult == ERROR_SUCCESS) {
//	    write_log("Success! Key Created!\n");
	    const char info[] = "URL:LiliTun remote Link\0";
	    lResult = RegSetValueEx(hKey, "" , 0, REG_SZ, (BYTE*) info, sizeof(info));
	    if (lResult == ERROR_SUCCESS) {
		const char info[] = "\0";
		lResult = RegSetValueEx(hKey, "URL Protocol", 0, REG_SZ, (BYTE*) info, sizeof(info));
	    }
	    if (lResult == ERROR_SUCCESS) {
		write_log("Success! Value Set!\n");
	    } else {
		write_log("Error %d\n", lResult);
		ret = 1;
	    }

	    RegCloseKey(hKey);
	}

	if (ret) {
	    return 0;
	}

	lResult = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\lilink\\shell\\open\\command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
	if (lResult == ERROR_SUCCESS) {
//	    write_log("Success! Key Created!\n");
	    char info[PATH_MAX];
	    snprintf(info, sizeof(info), "\"%s\" \"%%1\"\0", app);
	    lResult = RegSetValueEx(hKey, "" , 0, REG_SZ, (BYTE*) info, strlen(info) + 1);
	    if (lResult == ERROR_SUCCESS) {
		write_log("Success! Value Set!\n");
	    } else {
		write_log("Error %d\n", lResult);
		ret = 1;
	    }

	    RegCloseKey(hKey);
	}

	if (ret) {
	    return 0;
	}

	return 1;
}
#endif

static int parse_parameters(char *param)
{
    fprintf(stderr, "LINK [%s]\n", param);

    if (!strncmp(param, x_scheme_handler, strlen(x_scheme_handler))) {
	if (!extract_json_params(param + strlen(x_scheme_handler))) {
	    write_log("Unknown scheme!\n");

	    return 0;
	}

	write_log("- requestType      : %s\n", json_requestType);
	write_log("- appServerUrl     : %s\n", json_appServerUrl);
	write_log("- controlServerUrl : %s\n", json_controlServerUrl);
	write_log("- controlServerPort: %d\n", json_controlServerPort);
	write_log("- sessionId        : %s\n", json_sessionId);

	return 1;
    }

    write_log("Unknown scheme!\n");

    return 0;
}

#ifdef __APPLE__
void Set_URL_Handler(void (*cb)(const char *));

static void url_handler(const char *str)
{
    write_log("url [%s]\n", str);

    if (!parse_parameters((char *)str)) {
	exit(1);
    }
}
#endif

int main(int argc, char *argv[])
{
#if 0
{
    int fd = open("/Users/sash/projector.log", O_RDWR | O_APPEND | O_CREAT);
    if (fd < 0) {
	fprintf(stderr, "Cannot open projector.log!\n");
	exit(1);
    } else {
	if (dup2(fd, STDERR_FILENO) < 0) {
	    fprintf(stderr, "Cannot redirect stderr!\n");
//	exit(1);
	}
    }

    fprintf(stderr, "started [%s] %s %d\n", argv[0], argv[1], argc);
}
#endif

#ifdef _WIN32
    fprintf(stderr, "Registering URI scheme\n");
    register_uri_scheme(argv[0]);
#endif

#ifdef __APPLE__
	Set_URL_Handler(url_handler);
#endif
    if (argc > 1 && parse_parameters(argv[1])) {
	argc--;
	argv++;
#ifdef __APPLE__
    }
#endif


	Fl_Double_Window *win_main = init_gui();

	win_main->resize(win_main->x(), win_main->y(), win_main->w(), 35);

	win_main->show(argc, argv);

#ifdef __APPLE__
	write_log("Start app!\n");

	if (!json_requestType) {
	    fprintf(stderr, "No parameters, exit...\n");
	    return 0;
	}
#endif

	Fl::run();
#ifndef __APPLE__
    }
#endif

    return 0;
}
