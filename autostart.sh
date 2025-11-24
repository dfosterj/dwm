#!/bin/sh

# Find wallpaper - try multiple locations
WALLPAPER=""
# Try relative to dwm source directory (if DWM_DIR is set)
if [ -n "$DWM_DIR" ] && [ -f "$DWM_DIR/wallpaper/drwp1.jpeg" ]; then
	WALLPAPER="$DWM_DIR/wallpaper/drwp1.jpeg"
# Try common locations
elif [ -f "$HOME/.local/share/dwm/wallpaper/drwp1.jpeg" ]; then
	WALLPAPER="$HOME/.local/share/dwm/wallpaper/drwp1.jpeg"
elif [ -f "$HOME/.dwm/wallpaper/drwp1.jpeg" ]; then
	WALLPAPER="$HOME/.dwm/wallpaper/drwp1.jpeg"
# Try to find it relative to common dwm install locations
elif [ -f "/usr/local/share/dwm/wallpaper/drwp1.jpeg" ]; then
	WALLPAPER="/usr/local/share/dwm/wallpaper/drwp1.jpeg"
fi

# Set wallpaper if found
if [ -n "$WALLPAPER" ] && [ -f "$WALLPAPER" ]; then
	if command -v feh >/dev/null 2>&1; then
		feh --bg-scale "$WALLPAPER" &
	elif command -v nitrogen >/dev/null 2>&1; then
		nitrogen --set-scaled "$WALLPAPER" &
	elif command -v xwallpaper >/dev/null 2>&1; then
		xwallpaper --zoom "$WALLPAPER" &
	fi
fi

# Status bar loop: battery, volume, clock
while true; do
	# Battery
	batt=""
	if [ -d /sys/class/power_supply/BAT0 ]; then
		cap=$(cat /sys/class/power_supply/BAT0/capacity 2>/dev/null)
		stat=$(cat /sys/class/power_supply/BAT0/status 2>/dev/null)
		symbol="⚡"
		[ "$stat" = "Charging" ] && symbol="🔌"
		[ "$stat" = "Discharging" ] && symbol="🔋"
		batt="$symbol $cap%"
	fi

	# PulseAudio volume
	vol=""
	if command -v pactl >/dev/null 2>&1; then
		sink=$(pactl info 2>/dev/null | grep "Default Sink" | cut -d' ' -f3)
		if [ -n "$sink" ]; then
			mute=$(pactl get-sink-mute "$sink" 2>/dev/null | grep -o "yes\|no")
			if [ "$mute" = "yes" ]; then
				vol="🔇 Muted"
			else
				pct=$(pactl get-sink-volume "$sink" 2>/dev/null | head -1 | awk '{print $5}' | tr -d '%')
				[ -n "$pct" ] && vol="🔊 $pct%"
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
			wifi="📶 $ssid"
		else
			wifi="📶 Off"
		fi
	fi

	# Clock
	time="$(date '+%Y-%m-%d %H:%M:%S')"

	# Update dwm bar
	xsetroot -name "${batt} | ${wifi} | ${vol} | ${time}"
	sleep 1
done


# set keyboard layout
setxkbmap -layout us

