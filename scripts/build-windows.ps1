param(
    [string]$BuildDirName = "build-windows",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Release",
    [string]$Generator = "",
    [string]$CMakeMakeProgram = ""
)

$ErrorActionPreference = "Stop"

# Build the game for Windows using only project-relative paths.
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = (Resolve-Path (Join-Path $ScriptDir "..")).Path
$BuildDir = Join-Path $RootDir $BuildDirName

# Choose generator if provided or prefer Ninja/MinGW if available
if (-not [string]::IsNullOrEmpty($Generator)) {
    $genArgs = "-G `"$Generator`""
} else {
    $haveNinja = (Get-Command ninja -ErrorAction SilentlyContinue) -ne $null
    if ($haveNinja) {
        $genArgs = '-G "Ninja"'
    } else {
        # Fallback to MinGW Makefiles if mingw in PATH
        $haveMingwGcc = (Get-Command gcc -ErrorAction SilentlyContinue) -ne $null
        if ($haveMingwGcc) {
            $genArgs = '-G "MinGW Makefiles"'
        } else {
            $genArgs = ""
        }
    }
}

# Validate explicit generator choice
if (-not [string]::IsNullOrEmpty($Generator)) {
    if ($genArgs -match 'Ninja' -and (Get-Command ninja -ErrorAction SilentlyContinue) -eq $null) {
        Write-Error "Requested generator 'Ninja' but 'ninja' executable not found in PATH. Install Ninja or pick a different generator."
        exit 1
    }
    if ($genArgs -match 'MinGW Makefiles') {
        # If user requested MinGW Makefiles, ensure a make program is available or was provided
        $haveMake = (Get-Command mingw32-make -ErrorAction SilentlyContinue) -ne $null -or (Get-Command make -ErrorAction SilentlyContinue) -ne $null
        if (-not $haveMake -and [string]::IsNullOrEmpty($CMakeMakeProgram)) {
            Write-Error "Generator 'MinGW Makefiles' selected but no make program found and -CMakeMakeProgram not provided. Provide path to mingw32-make.exe or install a make program."
            exit 1
        }
    }
}

# Allow overriding the make program (e.g., mingw32-make)
$extraArgs = ""
if (-not [string]::IsNullOrEmpty($CMakeMakeProgram)) {
    $extraArgs = "-DCMAKE_MAKE_PROGRAM=`"$CMakeMakeProgram`""
}

# Detect and clean stale CMake cache if generator changed
$cacheFile = Join-Path $BuildDir "CMakeCache.txt"
if (Test-Path $cacheFile) {
    $cacheContent = Get-Content $cacheFile | Out-String
    # Simple check: if cache exists and generator name doesn't match, clean it
    $isNinjaCached = $cacheContent -match "CMAKE_GENERATOR:INTERNAL.*Ninja"
    $isMingwCached = $cacheContent -match "CMAKE_GENERATOR:INTERNAL.*MinGW"
    
    $isNinjaRequested = $genArgs -match 'Ninja'
    $isMingwRequested = $genArgs -match 'MinGW'
    
    # Mismatch: clean build directory
    if (($isNinjaCached -and -not $isNinjaRequested) -or 
        ($isMingwCached -and -not $isMingwRequested) -or
        (-not $isNinjaCached -and -not $isMingwCached -and ($isNinjaRequested -or $isMingwRequested))) {
        Write-Host "CMake cache generator mismatch detected. Cleaning build directory..."
        Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue
        New-Item -ItemType Directory $BuildDir -Force | Out-Null
    }
}

Write-Host "Configuring build: GeneratorArgs=$genArgs BuildType=$BuildType"
cmake -S $RootDir -B $BuildDir $genArgs "-DCMAKE_BUILD_TYPE=$BuildType" $extraArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed. See messages above."
    exit $LASTEXITCODE
}

if ($genArgs -match "Visual Studio") {
    cmake --build $BuildDir --config $BuildType
} else {
    # For single-config generators (Ninja / MinGW Makefiles)
    $cpuCount = [int]([Environment]::ProcessorCount)
    cmake --build $BuildDir -- -j $cpuCount
}

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit $LASTEXITCODE
}

Write-Host "Build complete: $RootDir\bin\GAME_APPLICATION.exe"
