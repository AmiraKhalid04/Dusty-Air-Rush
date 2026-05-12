param(
    [string]$BuildDirName = "build-windows",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

# Build the game for Windows using only project-relative paths.
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = (Resolve-Path (Join-Path $ScriptDir "..")).Path
$BuildDir = Join-Path $RootDir $BuildDirName

cmake -S $RootDir -B $BuildDir -DCMAKE_BUILD_TYPE=$BuildType
cmake --build $BuildDir --config $BuildType

Write-Host "Build complete: $RootDir\bin\GAME_APPLICATION.exe"
