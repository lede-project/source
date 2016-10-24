#!/bin/sh

if [ "$ACTION" = "pressed" -a "$BUTTON" = "wps" ]; then
	cd /var/run/hostapd
	for socket in *; do
		START_CLI=1
		PID_FILE=/var/run/hostapd_cli-wpsled-$socket.pid
		[ -S "$socket" ] || continue
		[ -f "$PID_FILE" ] && [ -d /proc/$(cat "$PID_FILE") ] && START_CLI=0
		[ "$START_CLI" -eq 1 ] && hostapd_cli -i "$socket" -P "$PID_FILE" -a "/lib/hostapd_wps_led.sh" -B && sleep 1
		[ x$(hostapd_cli -i "$socket" wps_pbc) != x"OK" ] && kill $(cat "$PID_FILE") && rm -f "$PID_FILE"
	done
fi

return 0
