#!/bin/sh /etc/rc.common

START=49

start () {

    if [ -e /etc/config/uhttpd ] && `! grep -qs osjs /etc/config/uhttpd`
    then
    	# make the webserver ponting to /osjs instead of /www
    	sed -i -e 's/www/osjs\/dist/g' /etc/config/uhttpd
        # Restarting uHttp server
    	# /etc/init.d/uhttpd restart
    fi

    if [ ! -d /osjs/dist/luci ]
    then
    	# Linking LUci
    	mkdir -p /osjs/dist/luci
    	ln -sf /www/index.html /osjs/dist/luci/index.html
    	ln -sf /www/luci-static /osjs/dist/luci-static
    	ln -sf /www/cgi-bin/luci /osjs/dist/cgi-bin/luci
    fi

}
