#!/bin/bash

if [ -e /cert/cert.pem -a -e /cert/privkey.pem ]; then

    if [ ! -e /cert/merged.pem ]; then
	cat /cert/cert.pem /cert/privkey.pem > /cert/merged.pem
    fi

    echo "Hostname: $HOST_NAME"

    sed -i -e "s|getenv('HOST_NAME')|'$HOST_NAME'|g" /var/www/html/desktop.php

    mkdir -p /var/run/lighttpd
    chown www-data:www-data /var/run/lighttpd

    mkdir -p /var/lib/controlserver
    chown www-data:www-data /var/lib/controlserver

    lighttpd -f /etc/lighttpd/lighttpd.conf

    controlserver
else
    echo "ERROR: copy cert.pem and privkey.pem to cert/ directory!"

    exit 1
fi
