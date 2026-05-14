#!/usr/bin/env bash
set -euo pipefail

# Create a portable Linux bundle and a .tar.gz archive.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR_NAME="${1:-build-linux}"
BUILD_TYPE="${2:-Release}"
DIST_DIR="${ROOT_DIR}/dist"
ARCH="$(uname -m)"
BUNDLE_NAME="Dusty-Air-Rush-linux-${ARCH}"
BUNDLE_DIR="${DIST_DIR}/${BUNDLE_NAME}"
ARCHIVE_PATH="${DIST_DIR}/${BUNDLE_NAME}.tar.gz"

"${SCRIPT_DIR}/build-linux.sh" "${BUILD_DIR_NAME}" "${BUILD_TYPE}"

mkdir -p "${DIST_DIR}"
rm -rf "${BUNDLE_DIR}"
mkdir -p "${BUNDLE_DIR}/bin"

cp "${ROOT_DIR}/bin/GAME_APPLICATION" "${BUNDLE_DIR}/bin/"
cp -a "${ROOT_DIR}/assets" "${BUNDLE_DIR}/assets"
cp -a "${ROOT_DIR}/config" "${BUNDLE_DIR}/config"
cp -a "${ROOT_DIR}/scoreboard.txt" "${BUNDLE_DIR}/scoreboard.txt"

cat > "${BUNDLE_DIR}/run.sh" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# Pass optional arguments through to the executable.
./bin/GAME_APPLICATION "$@"
EOF

chmod +x "${BUNDLE_DIR}/run.sh"

cat > "${BUNDLE_DIR}/install-shortcuts.sh" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

# Install a launcher in the app menu and optionally on the Desktop.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_NAME="Dusty Air Rush"
DESKTOP_FILE_NAME="dusty-air-rush.desktop"

APP_MENU_DIR="${XDG_DATA_HOME:-${HOME}/.local/share}/applications"
DESKTOP_DIR="${XDG_DESKTOP_DIR:-${HOME}/Desktop}"
if command -v xdg-user-dir >/dev/null 2>&1; then
	XDG_DESKTOP_FROM_CMD="$(xdg-user-dir DESKTOP 2>/dev/null || true)"
	if [[ -n "${XDG_DESKTOP_FROM_CMD}" ]]; then
		DESKTOP_DIR="${XDG_DESKTOP_FROM_CMD}"
	fi
fi

desktop_escape_string() {
	local value="$1"
	value="${value//\\/\\\\}"
	# Newlines are invalid in desktop entry values.
	value="${value//$'\n'/ }"
	printf '%s' "${value}"
}

desktop_escape_exec_arg() {
	local value="$1"
	value="${value//\\/\\\\}"
	value="${value//\"/\\\"}"
	value="${value//\$/\\\$}"
	value="${value//\`/\\\`}"
	printf '"%s"' "${value}"
}

ICON_CANDIDATES=(
	"${SCRIPT_DIR}/assets/icons/game-icon.png"
	"${SCRIPT_DIR}/assets/icons/game-icon.ico"
	"${SCRIPT_DIR}/assets/textures/menu.png"
	"${SCRIPT_DIR}/assets/textures/cup.png"
	"${SCRIPT_DIR}/assets/textures/dusty-win.png"
)

ICON_PATH=""
for icon in "${ICON_CANDIDATES[@]}"; do
	if [[ -f "${icon}" ]]; then
		ICON_PATH="${icon}"
		break
	fi
done

mkdir -p "${APP_MENU_DIR}"

APP_MENU_FILE="${APP_MENU_DIR}/${DESKTOP_FILE_NAME}"
ESCAPED_RUN_PATH="$(desktop_escape_exec_arg "${SCRIPT_DIR}/run.sh")"
ESCAPED_WORKDIR="$(desktop_escape_string "${SCRIPT_DIR}")"
ESCAPED_ICON_PATH="$(desktop_escape_string "${ICON_PATH}")"

cat > "${APP_MENU_FILE}" << DESKTOP_EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=${APP_NAME}
Comment=Fly through rings and dodge tornados in Dusty Air Rush
Exec=${ESCAPED_RUN_PATH}
Path=${ESCAPED_WORKDIR}
Icon=${ESCAPED_ICON_PATH}
Terminal=false
Categories=Game;
StartupNotify=true
DESKTOP_EOF

chmod +x "${APP_MENU_FILE}"

if [[ -d "${DESKTOP_DIR}" ]]; then
	DESKTOP_FILE_PATH="${DESKTOP_DIR}/${DESKTOP_FILE_NAME}"
	cp "${APP_MENU_FILE}" "${DESKTOP_FILE_PATH}"
	chmod +x "${DESKTOP_FILE_PATH}"
	echo "Desktop shortcut created: ${DESKTOP_FILE_PATH}"
else
	echo "Desktop folder not found at ${DESKTOP_DIR}."
	echo "App menu shortcut created only: ${APP_MENU_FILE}"
fi

if command -v update-desktop-database >/dev/null 2>&1; then
	update-desktop-database "${APP_MENU_DIR}" >/dev/null 2>&1 || true
fi

echo "App menu shortcut created: ${APP_MENU_FILE}"
EOF

cat > "${BUNDLE_DIR}/uninstall-shortcuts.sh" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

DESKTOP_FILE_NAME="dusty-air-rush.desktop"
APP_MENU_DIR="${XDG_DATA_HOME:-${HOME}/.local/share}/applications"
DESKTOP_DIR="${XDG_DESKTOP_DIR:-${HOME}/Desktop}"
if command -v xdg-user-dir >/dev/null 2>&1; then
	XDG_DESKTOP_FROM_CMD="$(xdg-user-dir DESKTOP 2>/dev/null || true)"
	if [[ -n "${XDG_DESKTOP_FROM_CMD}" ]]; then
		DESKTOP_DIR="${XDG_DESKTOP_FROM_CMD}"
	fi
fi

rm -f "${APP_MENU_DIR}/${DESKTOP_FILE_NAME}"
rm -f "${DESKTOP_DIR}/${DESKTOP_FILE_NAME}"

if command -v update-desktop-database >/dev/null 2>&1; then
	update-desktop-database "${APP_MENU_DIR}" >/dev/null 2>&1 || true
fi

echo "Removed launcher files (if they existed)."
EOF

chmod +x "${BUNDLE_DIR}/install-shortcuts.sh" "${BUNDLE_DIR}/uninstall-shortcuts.sh"

tar -czf "${ARCHIVE_PATH}" -C "${DIST_DIR}" "${BUNDLE_NAME}"

echo "Bundle directory: ${BUNDLE_DIR}"
echo "Archive created: ${ARCHIVE_PATH}"
