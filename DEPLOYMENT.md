# Creating Executables with Icons - Documentation

## Overview
This document explains how to create executable desktop and menu icons for **Dusty - Air Rush** on Linux and Windows.

---

## Linux Setup

### Prerequisites
- Game icon: `assets/icons/game-icon.png` (256x256 pixels recommended)
- Compiled binary in `bin/`

### Method 1: System Installation (Recommended for Distribution)

#### Step 1: Build with CMake
```bash
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
```

This will:
- Compile the game
- Generate `.desktop` file with name "Dusty - Air Rush"
- Install icon to `~/.local/share/icons/`
- Register in application menu

The game will appear in Activities/Applications menu.

#### Step 2: Add to Desktop (Optional)
Copy the `.desktop` file to your desktop:
```bash
cp ~/.local/share/applications/game.desktop ~/Desktop/"Dusty - Air Rush.desktop"
chmod +x ~/Desktop/"Dusty - Air Rush.desktop"
```

Then double-click to launch, or right-click → "Run as Program".

---

### Method 2: AppImage (Portable - Works on Any Linux)

#### Step 1: Build AppImage
```bash
cd /path/to/gfx-project
./build-appimage.sh
```

Output: `Dusty-Air-Rush-x86_64.AppImage` (~5 MB)

#### Step 2: Add to Desktop/Applications (Optional)
```bash
./install-desktop.sh
```

This adds:
- Desktop icon (clickable)
- Application menu entry (searchable)

#### Step 3: Share/Distribute
- No installation needed
- Works on Ubuntu, Fedora, Arch, and most Linux distros
- Just share the `.AppImage` file

**Example:**
```bash
./Dusty-Air-Rush-x86_64.AppImage
```

---

## Windows Setup

### Prerequisites
- Game icon: `assets/icons/game-icon.ico` (multiple resolutions: 16×16, 32×32, 48×48, 256×256)
- Visual Studio 2019+ or MinGW with CMake

### Step 1: Create Windows Icon Resource

The `CMakeLists.txt` automatically generates a resource file (`game.rc`) from your `.ico` file.

### Step 2: Build on Windows
```bash
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

Output: `bin/GAME_APPLICATION.exe` with embedded icon

### Step 3: Create Windows Desktop Shortcut

Create a `.lnk` (shortcut) file:

**Method A: Manual (Windows GUI)**
1. Right-click `GAME_APPLICATION.exe`
2. Select "Send To" → "Desktop (create shortcut)"
3. Rename to "Dusty - Air Rush"
4. Right-click → Properties → Change Icon → select `assets/icons/game-icon.ico`

**Method B: PowerShell Script**
```powershell
$WshShell = New-Object -ComObject WScript.Shell
$ShortcutPath = "$env:USERPROFILE\Desktop\Dusty - Air Rush.lnk"
$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = "C:\path\to\bin\GAME_APPLICATION.exe"
$Shortcut.WorkingDirectory = "C:\path\to\gfx-project"
$Shortcut.IconLocation = "C:\path\to\assets\icons\game-icon.ico"
$Shortcut.Save()
```

---

## Cross-Distro Compatibility

### Direct Binary Execution ❌
- **Problem**: Compiled on Ubuntu may not work on other distros
- **Reason**: glibc/libstdc++ version mismatches

### AppImage ✅ Recommended
- **Works on**: Ubuntu, Fedora, Arch, Debian, OpenSUSE, etc.
- **Requirements**: GLIBC 2.29+ (all modern distros)
- **Advantage**: Single file, no installation needed

### Snap (Alternative)
- **Works on**: Ubuntu, Fedora, Arch (via AUR)
- **Config**: `build-snap.yaml` included
- Build: `snapcraft`

### Static Linking (For Maximum Compatibility)
- Build with all libraries statically linked
- **Trade-off**: Larger executable (~50+ MB)
- **Advantage**: Works on very old systems

---

## File Structure

```
gfx-project/
├── CMakeLists.txt              # Main build config (generates .desktop on install)
├── AppRun                       # AppImage entry point
├── build-appimage.sh            # Script to build AppImage
├── build-snap.yaml              # Snap package config
├── assets/
│   └── icons/
│       ├── game-icon.png        # Linux icon
│       └── game-icon.ico        # Windows icon
├── bin/
│   └── GAME_APPLICATION         # Compiled binary (Linux)
└── build/
    ├── game.desktop             # Generated desktop entry
    └── game.rc                  # Generated Windows resource file
```

---

## Troubleshooting

### Desktop Icon Doesn't Launch (Linux)

**Symptom**: Double-click does nothing

**Solution 1**: Use Activities menu instead (Windows key + search)

**Solution 2**: Set executable permissions
```bash
chmod +x ~/Desktop/"Dusty - Air Rush.desktop"
```

**Solution 3**: Open with terminal to see errors
```bash
cd ~/Desktop && bash "Dusty - Air Rush.desktop"
```

### Icon Not Showing

**Linux**:
```bash
# Refresh icon cache
update-desktop-database ~/.local/share/applications/
gtk-update-icon-cache ~/.local/share/icons/hicolor/
```

**Windows**: Right-click → Properties → Change Icon (select .ico file)

### "Couldn't open file: config/app.jsonc"

**Cause**: Working directory is wrong

**Fix**: Ensure launcher changes to project directory:
```bash
cd /path/to/gfx-project && ./GAME_APPLICATION
```

---

## Quick Reference

| Task | Command |
|------|---------|
| Build locally | `cd build && cmake .. && make` |
| Build AppImage | `./build-appimage.sh` |
| Install to system | `cd build && cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local && make install` |
| Add to desktop | `./install-desktop.sh` (copies .desktop + icon) |
| Run game (Linux) | `./bin/GAME_APPLICATION` or press Super key, search "Dusty" |
| Run game (Windows) | `bin/GAME_APPLICATION.exe` or click desktop shortcut |
| Run AppImage | `./Dusty-Air-Rush-x86_64.AppImage` |

---

## Distribution Checklist

**For Linux Users:**
- [ ] Build AppImage: `./build-appimage.sh`
- [ ] Test: `./Dusty-Air-Rush-x86_64.AppImage`
- [ ] Share: `Dusty-Air-Rush-x86_64.AppImage` file
- [ ] Users can double-click or run from terminal

**For Windows Users:**
- [ ] Build executable: CMake + Visual Studio
- [ ] Create shortcut with icon
- [ ] Share: Full project folder or `.exe` + config files

**For Distribution Packages:**
- [ ] Linux: Use `build-appimage.sh` for universal .AppImage
- [ ] Windows: Create installer with NSIS or WiX
- [ ] Mac: Create `.dmg` with `.icns` icon (not covered here)

---

## Additional Resources

- [AppImage Documentation](https://docs.appimage.org/)
- [Desktop Entry Specification](https://specifications.freedesktop.org/desktop-entry-spec/)
- [Windows Icon Format](https://docs.microsoft.com/en-us/windows/win32/menurc/about-iconography)
- [CMake Desktop Integration](https://cmake.org/cmake/help/latest/guide/tutorial/Packaging%20an%20Installed%20Project.html)
