#!/bin/bash
# Build AppImage for Dusty - Air Rush

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${PROJECT_DIR}/bin"
BUILD_DIR="${PROJECT_DIR}/build"
APPRUN_FILE="${PROJECT_DIR}/AppRun"
ICON_FILE="${PROJECT_DIR}/assets/icons/game-icon.png"

# Check prerequisites
if [ ! -f "$APPRUN_FILE" ]; then
    echo "Error: AppRun script not found at $APPRUN_FILE"
    exit 1
fi

if [ ! -f "$ICON_FILE" ]; then
    echo "Error: Icon file not found at $ICON_FILE"
    exit 1
fi

# Check if binary exists
if [ ! -f "$BIN_DIR/GAME_APPLICATION" ]; then
    echo "Error: Binary not found at $BIN_DIR/GAME_APPLICATION"
    exit 1
fi

# Download appimagetool if not present
if ! command -v appimagetool &> /dev/null; then
    echo "Downloading appimagetool..."
    wget -q https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -O /tmp/appimagetool
    chmod +x /tmp/appimagetool
    APPIMAGETOOL="/tmp/appimagetool"
else
    APPIMAGETOOL="appimagetool"
fi

# Create AppDir structure
APPDIR="${BUILD_DIR}/AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR"

# Copy files
echo "Setting up AppImage directory..."
cp "${BIN_DIR}/GAME_APPLICATION" "$APPDIR/"
cp "$APPRUN_FILE" "$APPDIR/"
cp "$ICON_FILE" "$APPDIR/game-icon.png"
cp "${BUILD_DIR}/"*.desktop "$APPDIR/" 2>/dev/null || true

# Make AppRun executable
chmod +x "$APPDIR/AppRun"

# Build AppImage
echo "Building AppImage..."
APPIMAGE_NAME="Dusty-Air-Rush-x86_64.AppImage"
"$APPIMAGETOOL" "$APPDIR" "${PROJECT_DIR}/${APPIMAGE_NAME}"

# Make it executable
chmod +x "${PROJECT_DIR}/${APPIMAGE_NAME}"

echo "✓ AppImage created: ${PROJECT_DIR}/${APPIMAGE_NAME}"
echo "You can now double-click or execute: ./${APPIMAGE_NAME}"
