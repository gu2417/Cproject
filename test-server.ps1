# Test Chat Server PowerShell Script
# Usage: .\test-server.ps1 [duration]

param(
    [int]$Duration = 10
)

# Configuration
$BASH_EXE = "C:\msys64\usr\bin\bash.exe"
$SERVER_EXE = "src\server\chat_server.exe"

# Check prerequisites
if (!(Test-Path $BASH_EXE)) {
    Write-Host "[ERROR] MSYS2 not found. Please install from https://www.msys2.org/" -ForegroundColor Red
    exit 1
}

if (!(Test-Path $SERVER_EXE)) {
    Write-Host "[ERROR] chat_server.exe not found!" -ForegroundColor Red
    Write-Host "[ERROR] Please build first: .\build.ps1" -ForegroundColor Red
    exit 1
}

# Test
Write-Host ""
Write-Host "[TEST] Chat Server Test" -ForegroundColor Green
Write-Host "[TEST] Starting server on port 8080 for $Duration seconds..." -ForegroundColor Green
Write-Host ""

$CMD = @"
export PATH=/mingw64/bin:/mingw64/lib/lib:/usr/bin:/bin:`$PATH
cd /c/Users/nialg/test/chat_program
timeout $Duration ./src/server/chat_server.exe 2>&1 || true
"@

& $BASH_EXE -l -c $CMD

Write-Host ""
Write-Host "[TEST] Server test completed." -ForegroundColor Green
Write-Host ""
