@echo off
REM Test Chat Server
REM Usage: test-server.bat [duration]

setlocal enabledelayedexpansion

echo.
echo [TEST] Chat Server Test
echo [TEST] Starting server on port 8080...
echo.

REM Default duration: 10 seconds
set "DURATION=%1"
if "!DURATION!"=="" set "DURATION=10"

REM Check if server exists
if not exist "src\server\chat_server.exe" (
    echo [ERROR] chat_server.exe not found!
    echo [ERROR] Please build first: build.bat
    exit /b 1
)

REM Run server with timeout
set "BASH_PATH=C:\msys64\usr\bin\bash.exe"
call "!BASH_PATH!" -l -c "export PATH=/mingw64/bin:/usr/bin:/bin:$PATH && cd /c/Users/nialg/test/chat_program && timeout !DURATION! ./src/server/chat_server.exe 2>&1 || true"

echo.
echo [TEST] Server test completed.
echo.
