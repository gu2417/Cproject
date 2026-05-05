@echo off
REM Chat Program Build Script for Windows Command Prompt
REM Usage: build.bat [clean]

setlocal enabledelayedexpansion

echo.
echo [BUILD] Chat Program Build Script
echo [BUILD] Platform: Windows
echo.

REM Check for MSYS2
if not exist "C:\msys64\usr\bin\bash.exe" (
    echo [ERROR] MSYS2 not found at C:\msys64
    echo [ERROR] Please install MSYS2 from https://www.msys2.org/
    exit /b 1
)

REM Parse arguments
set "CLEAN_ARG="
if "%1"=="clean" (
    set "CLEAN_ARG=clean"
    echo [BUILD] Clean mode enabled
)

REM Build command
set "BUILD_CMD=export PATH=/mingw64/bin:/usr/bin:/bin:$PATH"
set "BUILD_CMD=!BUILD_CMD! ^&^& export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig"
set "BUILD_CMD=!BUILD_CMD! ^&^& cd /c/Users/nialg/test/chat_program"

if not "!CLEAN_ARG!"=="" (
    set "BUILD_CMD=!BUILD_CMD! ^&^& make UNAME=MINGW64_NT-10.0-26200 clean"
)

set "BUILD_CMD=!BUILD_CMD! ^&^& make UNAME=MINGW64_NT-10.0-26200"

echo [BUILD] Executing build...
echo.

call "C:\msys64\usr\bin\bash.exe" -l -c "!BUILD_CMD!"

if %ERRORLEVEL% equ 0 (
    echo.
    echo [BUILD] Build completed successfully!
    
    REM Check binaries
    if exist "src\server\chat_server.exe" (
        for %%F in ("src\server\chat_server.exe") do (
            echo [BUILD] Server: %%F (%%~zF bytes^)
        )
    )
    
    if exist "src\client\chat_client.exe" (
        for %%F in ("src\client\chat_client.exe") do (
            echo [BUILD] Client: %%F (%%~zF bytes^)
        )
    )
    
    echo.
    echo [BUILD] Next steps:
    echo   - Run test-server.bat to start the server
    echo   - Run src\client\chat_client.exe to start the client
    echo.
    exit /b 0
) else (
    echo.
    echo [ERROR] Build failed!
    exit /b 1
)
