#!/bin/bash

# Path where Xsession desktop files go
XSESSION_DIR="/usr/share/xsessions"
DESKTOP_FILE="$XSESSION_DIR/dwm.desktop"

# Ensure the directory exists
if [ ! -d "$XSESSION_DIR" ]; then
    echo "Directory $XSESSION_DIR does not exist. Are you running this on a system with a display manager?"
    exit 1
fi

echo "Creating $DESKTOP_FILE ..."

sudo tee "$DESKTOP_FILE" >/dev/null <<EOF
[Desktop Entry]
Name=dwm
Comment=Dynamic Window Manager
Exec=/usr/local/bin/dwm
Type=XSession
EOF

echo "dwm Xsession entry created at: $DESKTOP_FILE"

