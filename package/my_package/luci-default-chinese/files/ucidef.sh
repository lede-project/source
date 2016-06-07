#!/bin/sh
uci batch <<-EOF
	set luci.main.lang=zh_cn
	commit luci
EOF
