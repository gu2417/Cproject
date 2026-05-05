# Chat Program Build Script for Windows PowerShell
# Usage: .\build.ps1 [clean] [verbose]

param(
    [switch]$clean,
    [switch]$verbose
)

# Color output helper
function Write-Status {
    param([string]$message)
    Write-Host "[BUILD] $message" -ForegroundColor Green
}

function Write-Error {
    param([string]$message)
    Write-Host "[ERROR] $message" -ForegroundColor Red
}

function Write-Warning {
    param([string]$message)
    Write-Host "[WARN] $message" -ForegroundColor Yellow
}

# Configuration
$MSYS2_PATH = "C:\msys64"
$MINGW_BIN = "$MSYS2_PATH\mingw64\bin"
$BASH_EXE = "$MSYS2_PATH\usr\bin\bash.exe"

# Check prerequisites
Write-Status "Checking prerequisites..."

if (!(Test-Path $BASH_EXE)) {
    Write-Error "MSYS2 not found at $MSYS2_PATH"
    Write-Error "Please install MSYS2 from https://www.msys2.org/"
    exit 1
}

# Build command
$BUILD_CMD = @"
export PATH=$MINGW_BIN`:/usr/bin:/bin:`$PATH
export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig
cd /c/Users/nialg/test/chat_program
"@

if ($clean) {
    Write-Status "Cleaning build artifacts..."
    $BUILD_CMD += "`nmake UNAME=MINGW64_NT-10.0-26200 clean"
}

$BUILD_CMD += "`nmake UNAME=MINGW64_NT-10.0-26200"

if ($verbose) {
    $BUILD_CMD += " 2>&1"
} else {
    $BUILD_CMD += " 2>&1 | grep -E '^\[|error|Error|ERROR'"
}

# Execute build
Write-Status "Starting build..."
Write-Status "Command: make UNAME=MINGW64_NT-10.0-26200$(if ($clean) { ' clean' })"
Write-Host ""

& $BASH_EXE -l -c $BUILD_CMD
$BUILD_EXIT = $LASTEXITCODE

# Check results
Write-Host ""
if ($BUILD_EXIT -eq 0) {
    Write-Status "Build completed successfully!"
    
    # Check binaries
    if ((Test-Path "src\server\chat_server.exe") -and (Test-Path "src\client\chat_client.exe")) {
        Write-Status "Binaries generated:"
        Get-Item "src\server\chat_server.exe", "src\client\chat_client.exe" | ForEach-Object {
            $size = "{0:N0}" -f $_.Length
            Write-Host "  ✓ $($_.Name) ($size bytes)" -ForegroundColor Green
        }
    }
} else {
    Write-Error "Build failed with exit code $BUILD_EXIT"
    exit $BUILD_EXIT
}

Write-Host ""
Write-Status "Build script completed."
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Test server: .\test-server.ps1"
Write-Host "  2. Run client: .\src\client\chat_client.exe"
