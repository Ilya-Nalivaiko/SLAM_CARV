#!/bin/bash
set -e

mkdir -p ~/.vnc
echo "${X11VNC_P}" | /opt/TurboVNC/bin/vncpasswd -f > ~/.vnc/passwd
chmod 600 ~/.vnc/passwd

# Minimal Openbox config: no workspaces, no scroll switching. xterm
mkdir -p ~/.config/openbox
cat > ~/.config/openbox/menu.xml <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<openbox_menu xmlns="http://openbox.org/3.4/menu">
  <menu id="root-menu" label="Openbox Menu">
    <item label="Terminal">
      <action name="Execute"><command>xterm</command></action>
    </item>
    <item label="Reconfigure">
      <action name="Reconfigure"/>
    </item>
    <item label="Exit">
      <action name="Exit"/>
    </item>
  </menu>
</openbox_menu>
EOF
echo "xterm & xterm &" > ~/.config/openbox/autostart

/opt/TurboVNC/bin/vncserver :1 -geometry 1920x1080 -depth 16 -wm openbox
echo "[INFO] TurboVNC started on :1 with Openbox (no workspace switching)"
tail -f /dev/null
