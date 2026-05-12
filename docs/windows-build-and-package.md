# Windows Build And Executable Packaging Guide

This guide shows how to build Dusty Air Rush on Windows and package it as a portable executable bundle for other Windows PCs.

## 1) Install Requirements

- CMake 3.15+
- A C++17-capable compiler toolchain
- One of these setups:
  - Visual Studio 2022 with Desktop development with C++ workload
  - Visual Studio Build Tools 2022 with MSVC C++ tools
  - MinGW-w64 (if you prefer GCC on Windows)

PowerShell is used by the scripts in this guide.

## 2) Build The Executable

From the project root in PowerShell:

```powershell
./scripts/build-windows.ps1
```

Output executable:

- `bin/GAME_APPLICATION.exe`

Quick run check:

```powershell
./bin/GAME_APPLICATION.exe -c config/app.jsonc -f 10
```

## 3) Create A Portable Windows Bundle

From the project root:

```powershell
./scripts/package-windows.ps1
```

This creates:

- Bundle folder: `dist/Dusty-Air-Rush-windows-x64/`
- Compressed archive: `dist/Dusty-Air-Rush-windows-x64.zip`

Bundle contents:

- `bin/GAME_APPLICATION.exe`
- `assets/`
- `config/`
- `scoreboard.txt`
- `run.bat`
- `install-shortcuts.ps1`
- `uninstall-shortcuts.ps1`

## 4) Add Shortcut To Start Menu And Desktop

From inside the extracted bundle folder:

```powershell
powershell -ExecutionPolicy Bypass -File .\install-shortcuts.ps1
```

What this does:

- Creates a Start Menu shortcut in your user Programs folder
- Creates a Desktop shortcut on your user Desktop
- Uses icon `assets/icons/game-icon.ico` (falls back to `assets/icons/game-icon.png`, then executable icon)

To remove shortcuts later:

```powershell
powershell -ExecutionPolicy Bypass -File .\uninstall-shortcuts.ps1
```

## 5) Run On Another Windows PC

From inside the extracted bundle:

```powershell
.\run.bat
```

Optional arguments:

```powershell
.\run.bat -c config/app.jsonc
.\run.bat -f 300
```

## Notes

- No machine-specific absolute paths are hardcoded in the scripts.
- The game expects `assets/` and `config/` next to `run.bat` in the packaged layout.
- If SmartScreen appears, choose More info then Run anyway only if you trust the source.
