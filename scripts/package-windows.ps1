param(
    [string]$BuildDirName = "build-windows",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

# Create a portable Windows bundle and a .zip archive.
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = (Resolve-Path (Join-Path $ScriptDir "..")).Path
$DistDir = Join-Path $RootDir "dist"
$BundleName = "Dusty-Air-Rush-windows-x64"
$BundleDir = Join-Path $DistDir $BundleName
$ArchivePath = Join-Path $DistDir "$BundleName.zip"

& (Join-Path $ScriptDir "build-windows.ps1") -BuildDirName $BuildDirName -BuildType $BuildType

New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
if (Test-Path $BundleDir) {
    Remove-Item -Path $BundleDir -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $BundleDir "bin") -Force | Out-Null

Copy-Item -Path (Join-Path $RootDir "bin\GAME_APPLICATION.exe") -Destination (Join-Path $BundleDir "bin") -Force
Copy-Item -Path (Join-Path $RootDir "assets") -Destination (Join-Path $BundleDir "assets") -Recurse -Force
Copy-Item -Path (Join-Path $RootDir "config") -Destination (Join-Path $BundleDir "config") -Recurse -Force
Copy-Item -Path (Join-Path $RootDir "scoreboard.txt") -Destination (Join-Path $BundleDir "scoreboard.txt") -Force

$runBat = @'
@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM Pass optional arguments through to the executable.
"%SCRIPT_DIR%bin\GAME_APPLICATION.exe" %*
'@
Set-Content -Path (Join-Path $BundleDir "run.bat") -Value $runBat -NoNewline

$installShortcuts = @'
$ErrorActionPreference = "Stop"

# Install shortcuts in Start Menu and Desktop.
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$AppName = "Dusty Air Rush"
$ShortcutName = "Dusty Air Rush.lnk"
$RunTarget = Join-Path $ScriptDir "run.bat"
$WorkingDir = $ScriptDir

$IconCandidates = @(
    (Join-Path $ScriptDir "assets\icons\game-icon.ico"),
    (Join-Path $ScriptDir "assets\icons\game-icon.png"),
    (Join-Path $ScriptDir "bin\GAME_APPLICATION.exe")
)

$IconPath = $null
foreach ($icon in $IconCandidates) {
    if (Test-Path $icon) {
        $IconPath = $icon
        break
    }
}

$DesktopDir = [Environment]::GetFolderPath("Desktop")
$StartMenuPrograms = [Environment]::GetFolderPath("Programs")

$DesktopShortcut = Join-Path $DesktopDir $ShortcutName
$StartMenuShortcut = Join-Path $StartMenuPrograms $ShortcutName

$Shell = New-Object -ComObject WScript.Shell

$DesktopLink = $Shell.CreateShortcut($DesktopShortcut)
$DesktopLink.TargetPath = $RunTarget
$DesktopLink.WorkingDirectory = $WorkingDir
if ($IconPath) { $DesktopLink.IconLocation = $IconPath }
$DesktopLink.Save()

$StartLink = $Shell.CreateShortcut($StartMenuShortcut)
$StartLink.TargetPath = $RunTarget
$StartLink.WorkingDirectory = $WorkingDir
if ($IconPath) { $StartLink.IconLocation = $IconPath }
$StartLink.Save()

Write-Host "Desktop shortcut created: $DesktopShortcut"
Write-Host "Start Menu shortcut created: $StartMenuShortcut"
'@
Set-Content -Path (Join-Path $BundleDir "install-shortcuts.ps1") -Value $installShortcuts -NoNewline

$uninstallShortcuts = @'
$ErrorActionPreference = "Stop"

$ShortcutName = "Dusty Air Rush.lnk"
$DesktopDir = [Environment]::GetFolderPath("Desktop")
$StartMenuPrograms = [Environment]::GetFolderPath("Programs")

$DesktopShortcut = Join-Path $DesktopDir $ShortcutName
$StartMenuShortcut = Join-Path $StartMenuPrograms $ShortcutName

if (Test-Path $DesktopShortcut) {
    Remove-Item -Path $DesktopShortcut -Force
}
if (Test-Path $StartMenuShortcut) {
    Remove-Item -Path $StartMenuShortcut -Force
}

Write-Host "Removed shortcuts (if they existed)."
'@
Set-Content -Path (Join-Path $BundleDir "uninstall-shortcuts.ps1") -Value $uninstallShortcuts -NoNewline

if (Test-Path $ArchivePath) {
    Remove-Item -Path $ArchivePath -Force
}
Compress-Archive -Path $BundleDir -DestinationPath $ArchivePath -Force

Write-Host "Bundle directory: $BundleDir"
Write-Host "Archive created: $ArchivePath"
