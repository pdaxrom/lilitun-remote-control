#!/bin/bash

get_mime() {
    case $1 in
    *.css) echo "text/css";;
    *.html|*.htm) echo "text/html";;
    *.jpg|*.jpeg) echo "image/jpeg";;
    *.js) echo "text/javascript";;
    *.png) echo "image/png";;
    *.svg) echo "image/svg+xml";;
    *.txt) echo "text/plain";;
    *.zip) echo "application/x-compressed";;
    *) echo "application/octet-stream";;
    esac
}

ROOT=$1

shift

cd $ROOT

FILES=""
FILES_SZ=""

cat >/dev/stdout<<EOF
#include <string.h>
#include "pseudofs.h"

struct pseudofs {
    char *name;
    int len;
    char *mime;
    int offset;
} static pseudofs_files[] = {
EOF

OFFSET=0
while [ "$1" != "" ]; do
    if [ -f "$1" ]; then
	FILES="$FILES $1"
#	FILES_SZ="$FILES_SZ $(stat -c "%s" "$1")"
	SIZE=$(stat -c "%s" "$1")
	MIME=$(get_mime $1)
	echo "    { \"/$1\", $SIZE, \"$MIME\", $OFFSET }," >> /dev/stdout
	OFFSET=$((OFFSET + $SIZE))
    fi
    shift
done

cat >>/dev/stdout<<EOF
};

static const char pseudofs_data[] = {
EOF

for f in $FILES; do
    hexdump -v -e '"    " 16/1 "0x%02x, " "\n"' $f | sed 's/\0x  ,//g' >>/dev/stdout
done

cat >>/dev/stdout<<EOF
};

const char *pseudofs_get_file(char *name, int *len, const char **mime)
{
    for (int i = 0; i < sizeof(pseudofs_files) / sizeof(struct pseudofs); i++) {
	if (!strcmp(pseudofs_files[i].name, name)) {
	    *len = pseudofs_files[i].len;
	    *mime = pseudofs_files[i].mime;
	    return &pseudofs_data[pseudofs_files[i].offset];
	}
    }
    return NULL;
}
EOF
