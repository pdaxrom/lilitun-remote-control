FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive

COPY controlserver_1.0_amd64.deb /root/

ENV INSIDE_DOCKER="yes"

WORKDIR /root

RUN apt update && apt full-upgrade -y && apt install -y lighttpd php-cgi php-sqlite3 php-curl && \
    lighttpd-enable-mod rewrite fastcgi-php && \
    dpkg -i controlserver_1.0_amd64.deb || true && apt install -f -y && \
    apt install -y python3-numpy python3

COPY web/lighttpd.conf /etc/lighttpd/

COPY web/www /var/www/html

COPY web/startme.sh /root/

COPY release /var/www/html/downloads/

CMD ["/bin/bash", "-c", "/root/startme.sh"]
