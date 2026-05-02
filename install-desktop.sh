#!/bin/bash
# Install Dusty - Air Rush to desktop and application menu

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APPIMAGE="${PROJECT_DIR}/Dusty-Air-Rush-x86_64.AppImage"
ICON="${PROJECT_DIR}/assets/icons/game-icon.png"

if [ ! -f "$APPIMAGE" ]; then
    echo "Error: AppImage not found. Run ./build-appimage.sh first"
    exit 1
fi

if [ ! -f "$ICON" ]; then
    echo "Error: Icon not found at $ICON"
    exit 1
fi

# Make sure AppImage is executable
chmod +x "$APPIMAGE"

# Create desktop entry file
DESKTOP_FILE="$HOME/Desktop/Dusty - Air Rush.desktop"
APPS_DIR="$HOME/.local/share/applications"
APPS_ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"

# Create directories if they don't exist
mkdir -p "$HOME/Desktop"
mkdir -p "$APPS_DIR"
mkdir -p "$APPS_ICON_DIR"

# Copy icon to apps directory
cp "$ICON" "$APPS_ICON_DIR/game-icon.png"

# Create desktop file for application menu
cat > "${APPS_DIR}/dusty-air-rush.desktop" << 'EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=Dusty - Air Rush
Exec=/home/omar/cmp/year3/gfx/gfx-project/Dusty-Air-Rush-x86_64.AppImage
Icon=game-icon
Categories=Game;Graphics;
Terminal=false
EOF

# Create desktop file for desktop shortcuts
cat > "$DESKTOP_FILE" << 'EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=Dusty - Air Rush
Exec=/home/omar/cmp/year3/gfx/gfx-project/Dusty-Air-Rush-x86_64.AppImage
Icon=game-icon
Categories=Game;Graphics;
Terminal=false
EOF

# Make desktop file executable
chmod +x "$DESKTOP_FILE"
chmod +x "${APPS_DIR}/dusty-air-rush.desktop"

# Update icon cache
update-desktop-database "$APPS_DIR" 2>/dev/null || true
gtk-update-icon-cache "$APPS_ICON_DIR" 2>/dev/null || true

echo "✓ Added to desktop: $DESKTOP_FILE"
echo "✓ Added to application menu"
echo ""
echo "You should now see:"
echo "  • 'Dusty - Air Rush' icon on your desktop"
echo "  • 'Dusty - Air Rush' in your application menu (Activities)"
echo ""
echo "If the icon doesn't appear immediately, try relogging or restarting your desktop environment."
