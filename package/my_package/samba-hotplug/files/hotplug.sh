#!/bin/sh
#   Samba自动配置脚本，参考PandoraBox的脚本框架并重写。以下为原脚本作者信息：
#===================================================================================
#
#   D-Team Technology Co.,Ltd. ShenZhen
#   作者:Vic
# 
#
# 警告:对着屏幕的哥们,我们允许你使用此脚本，但不允许你抹去作者的信息,请保留这段话。
#===================================================================================
. /lib/functions.sh

set_samba(){
	section=$(echo $get_uuid | sed 's/-//g')

	uci set samba.${section}="sambashare"
	uci set samba.${section}.name="Disk_${device}"
	uci set samba.${section}.path="${mountpoint}"
	uci set samba.${section}.read_only="no"
	uci set samba.${section}.guest_ok="yes"
	uci commit samba
}
set_samba_path(){
	section=$(echo $get_uuid | sed 's/-//g')

	uci set samba.${section}.path="${mountpoint}"
	uci commit samba
}


device=`basename $DEVPATH`

case "$ACTION" in
	add)
	
       case "$device" in
                sd*) ;;
                md*) ;;
                hd*);;     
                mmcblk*);;  
                *) return;;
        esac   
        
	sleep 2
	mountpoint=`sed -ne "s|^[^ ]*/$device ||; T; s/ .*//p" /proc/self/mounts`
	get_uuid=`block info | grep "/dev/${device}" | awk -F "UUID=" '{print $2}'| awk -F "\"" '{print $2}'`
	have_uuid=$(uci show samba | grep -c "$get_uuid")
	[ "$have_uuid" = "0" ] && set_samba
	[ "$have_uuid" -gt "0" ] && set_samba_path
	/etc/init.d/samba restart
	;;
	remove)
	;;
esac
