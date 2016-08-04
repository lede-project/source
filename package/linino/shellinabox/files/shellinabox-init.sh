#!/bin/sh /etc/rc.common
# Copyright (C) 2006 OpenWrt.org

START=99
STOP=99

start () {

    if [ ! -e /etc/ssl/certs/certificate.pem ]
    then
        /usr/sbin/gen-cert
    fi

    /usr/sbin/shellinaboxd -s /:SSH --cert=/etc/ssl/certs -b --user=root -t

}

stop () {

    killall shellinaboxd 2> /dev/null

}
