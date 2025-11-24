# Autostart Setup Reference

Quick reference for setting up autostart functionality in dwm using the autostart patch.

## Overview

The autostart patch allows dwm to automatically execute scripts on startup. It looks for scripts in these directories (in order):
1. `$XDG_DATA_HOME/dwm` (if `$XDG_DATA_HOME` is set)
2. `$HOME/.local/share/dwm`
3. `$HOME/.dwm`

## Files

- **`autostart.sh`** - Main autostart script that runs in background
- **`install-autostart.sh`** - Installation script that copies files to the correct location

## Script Types

The patch supports two script types:
- **`autostart_blocking.sh`** - Runs before dwm enters handler loop (dwm waits for completion)
- **`autostart.sh`** - Runs in background (non-blocking)

## Installation

```bash
# From dwm source directory
./install-autostart.sh
```

This will:
1. Detect the appropriate installation directory (XDG or ~/.dwm)
2. Copy `autostart.sh` to that directory
3. Copy `wallpaper/` directory to that location
4. Make scripts executable

## Manual Installation

```bash
# Option 1: XDG directory (recommended)
mkdir -p ~/.local/share/dwm
cp autostart.sh ~/.local/share/dwm/
chmod +x ~/.local/share/dwm/autostart.sh
cp -r wallpaper ~/.local/share/dwm/

# Option 2: Traditional directory
mkdir -p ~/.dwm
cp autostart.sh ~/.dwm/
chmod +x ~/.dwm/autostart.sh
cp -r wallpaper ~/.dwm/
```

## Autostart Script Structure

The `autostart.sh` script typically contains:

1. **Wallpaper setup** - Sets background image
2. **Status bar loop** - Continuously updates `xsetroot -name` with system info
3. **Other background processes** - Any other startup tasks

### Example Structure

```bash
#!/bin/sh

# 1. Set wallpaper
WALLPAPER="path/to/wallpaper.jpg"
if [ -f "$WALLPAPER" ]; then
    feh --bg-scale "$WALLPAPER" &
fi

# 2. Status bar loop
while true; do
    # Gather system info
    batt="..."
    vol="..."
    time="$(date '+%Y-%m-%d %H:%M:%S')"
    
    # Update status bar
    xsetroot -name "$batt | $vol | $time"
    sleep 1
done
```

## Key Points

- **Script runs automatically** - No need to call it from `.xinitrc`
- **Background process** - The status bar loop runs forever (use `&` if needed)
- **Wallpaper path** - Script should handle finding wallpaper in multiple locations
- **Status updates** - Use `xsetroot -name` to update dwm status bar
- **Error handling** - Check for command existence before using (`command -v cmd`)

## Status Bar Format

The status bar is updated via:
```bash
xsetroot -name "status text here"
```

dwm reads this from the root window's `XA_WM_NAME` property.

## Testing

To test the autostart script manually:
```bash
~/.local/share/dwm/autostart.sh
# or
~/.dwm/autostart.sh
```

## Troubleshooting

- **Script not running**: Check it's executable (`chmod +x`)
- **Script not found**: Verify it's in the correct directory
- **Status not updating**: Check script is running (`ps aux | grep autostart`)
- **Wallpaper not setting**: Verify path and that feh/nitrogen/xwallpaper is installed

## Template Script

Basic template for `autostart.sh`:

```bash
#!/bin/sh

# Wallpaper setup
WALLPAPER="$HOME/.local/share/dwm/wallpaper/wallpaper.jpg"
if [ -f "$WALLPAPER" ] && command -v feh >/dev/null 2>&1; then
    feh --bg-scale "$WALLPAPER" &
fi

# Status bar loop
while true; do
    # Your status components here
    status="$(date '+%Y-%m-%d %H:%M:%S')"
    
    # Update status bar
    xsetroot -name "$status"
    sleep 1
done
```

## For Next Project

1. Copy `autostart.sh` template
2. Copy `install-autostart.sh` 
3. Modify `autostart.sh` for your needs
4. Run `./install-autostart.sh` to install
5. Rebuild dwm: `make clean install`

## install-autostart.sh Template

Basic template for installation script:

```bash
#!/bin/sh

DWM_SOURCE_DIR="$(cd "$(dirname "$0")" && pwd)"
AUTOSTART_SCRIPT="$DWM_SOURCE_DIR/autostart.sh"

# Determine installation directory
if [ -n "$XDG_DATA_HOME" ]; then
    INSTALL_DIR="$XDG_DATA_HOME/dwm"
elif [ -d "$HOME/.local/share" ]; then
    INSTALL_DIR="$HOME/.local/share/dwm"
else
    INSTALL_DIR="$HOME/.dwm"
fi

mkdir -p "$INSTALL_DIR"
cp "$AUTOSTART_SCRIPT" "$INSTALL_DIR/"
chmod +x "$INSTALL_DIR/autostart.sh"
echo "Installed to: $INSTALL_DIR"
```

