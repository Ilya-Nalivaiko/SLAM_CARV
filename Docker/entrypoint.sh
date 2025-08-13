#!/bin/bash
set -e

# Create VNC password
mkdir -p ~/.vnc
echo "${X11VNC_P}" | /opt/TurboVNC/bin/vncpasswd -f > ~/.vnc/passwd
chmod 600 ~/.vnc/passwd

# Start TurboVNC server with fluxbox as WM
/opt/TurboVNC/bin/vncserver :1 -geometry 1920x1080 -depth 16 -wm fluxbox

echo "[INFO] TurboVNC started on :1"

# Keep container alive
tail -f /dev/null
