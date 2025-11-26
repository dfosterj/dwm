#!/bin/sh

# Status bar loop: battery, volume, clock
while true; do
	# Battery
	batt=""
	if [ -d /sys/class/power_supply/BAT0 ]; then
		cap=$(cat /sys/class/power_supply/BAT0/capacity 2>/dev/null)
		stat=$(cat /sys/class/power_supply/BAT0/status 2>/dev/null)
		symbol="BAT"
		[ "$stat" = "Charging" ] && symbol="CHG"
		[ "$stat" = "Discharging" ] && symbol="BAT"
		batt="$symbol $cap%"
	fi

	# PulseAudio volume
	vol=""
	if command -v pactl >/dev/null 2>&1; then
		sink=$(pactl info 2>/dev/null | grep "Default Sink" | cut -d' ' -f3)
		if [ -n "$sink" ]; then
			mute=$(pactl get-sink-mute "$sink" 2>/dev/null | grep -o "yes\|no")
			if [ "$mute" = "yes" ]; then
				vol="VOL Muted"
			else
				pct=$(pactl get-sink-volume "$sink" 2>/dev/null | head -1 | awk '{print $5}' | tr -d '%')
				[ -n "$pct" ] && vol="VOL $pct%"
			fi
		fi
	fi

	# Wi-Fi
	wifi=""
	if command -v nmcli >/dev/null 2>&1; then
		wifi_state=$(nmcli -t -f WIFI g 2>/dev/null)
		if [ "$wifi_state" = "enabled" ]; then
			ssid=$(nmcli -t -f ACTIVE,SSID dev wifi 2>/dev/null | grep '^yes:' | cut -d':' -f2)
			[ -z "$ssid" ] && ssid="Disconnected"
			wifi="WIFI $ssid"
		else
			wifi="WIFI Off"
		fi
	fi

	# Clock
	time="$(date '+%Y-%m-%d %I:%M:%S %p')"

	# Update dwm bar
	xsetroot -name "${batt} | ${wifi} | ${vol} | ${time}"
	sleep 1
done


#startup procs
export GTK_THEME="Adwaita:dark"
dunst &
feh --bg-scale ./wallpaper/drwp1.jpeg  &


