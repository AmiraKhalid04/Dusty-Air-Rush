# Linux Build And Executable Packaging Guide

This guide shows how to build Dusty Air Rush on Linux and package it as a portable executable bundle for other Linux PCs.

## 1) Install Requirements

### Build tools

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config
```

### OpenGL/X11 development packages

```bash
sudo apt install -y libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
```

## 2) Build The Executable

From the project root:

```bash
chmod +x scripts/build-linux.sh scripts/package-linux.sh
./scripts/build-linux.sh
```

Output executable:

- `bin/GAME_APPLICATION`

Quick run check:

```bash
./bin/GAME_APPLICATION -c config/app.jsonc -f 10
```

## 3) Create A Portable Linux Bundle

From the project root:

```bash
./scripts/package-linux.sh
```

This creates:

- Bundle folder: `dist/Dusty-Air-Rush-linux-<arch>/`
- Compressed archive: `dist/Dusty-Air-Rush-linux-<arch>.tar.gz`

The bundle contains only relative paths:

- `bin/GAME_APPLICATION`
- `assets/`
- `config/`
- `scoreboard.txt`
- `run.sh`
- `install-shortcuts.sh`
- `uninstall-shortcuts.sh`

## 4) Add Launcher To App Menu And Desktop

From inside the extracted bundle folder:

```bash
chmod +x install-shortcuts.sh
./install-shortcuts.sh
```

What this does:

- Creates an app launcher in `~/.local/share/applications/dusty-air-rush.desktop`
- Creates a desktop shortcut in `~/Desktop/dusty-air-rush.desktop` (if the Desktop folder exists)
- Uses the icon at `assets/icons/game-icon.png` (falls back to texture icons only if missing)

To remove those launchers later:

```bash
chmod +x uninstall-shortcuts.sh
./uninstall-shortcuts.sh
```

## 5) Run On Another Linux PC

### On the target machine

1. Install runtime graphics/audio dependencies (usually already present on desktop Linux).
2. Extract the archive.
3. Run from inside the extracted folder:

```bash
chmod +x run.sh
./run.sh
```

Optional arguments:

```bash
./run.sh -c config/app.jsonc
./run.sh -f 300
```

## Notes

- No absolute paths are used; scripts resolve paths from their own location.
- The executable expects `assets/` and `config/` to be beside it in the bundle layout above.
- If you build on one distro and run on another, very old systems may need compatible runtime libraries.
