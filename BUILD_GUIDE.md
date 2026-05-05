# Chat Program Build & Test Guide

## Quick Start

### 1. Build (Windows)

**Option A: Command Prompt (Easiest)**
```cmd
cd chat_program
build.bat
```

**Option B: PowerShell**
```powershell
cd chat_program
.\build.ps1
```

**Option C: MSYS2 Bash**
```bash
cd chat_program
./build.sh
```

### 2. Build with Clean

**Command Prompt:**
```cmd
build.bat clean
```

**PowerShell:**
```powershell
.\build.ps1 -clean
```

**MSYS2 Bash:**
```bash
./build.sh clean
```

### 3. Test Server

**Command Prompt:**
```cmd
test-server.bat
```

Default: 10 seconds
```cmd
test-server.bat 5
```
Run for 5 seconds

**PowerShell:**
```powershell
.\test-server.ps1 -Duration 5
```

**MSYS2 Bash:**
```bash
export PATH=/mingw64/bin:/usr/bin:/bin:$PATH
timeout 10 ./src/server/chat_server.exe
```

### 4. Run Client

```cmd
src\client\chat_client.exe
```

## Prerequisites

- **MSYS2**: Required for build environment
  - Install from https://www.msys2.org/
  - Required packages: `make`, `mingw-w64-x86_64-toolchain`, `mingw-w64-x86_64-gtk4`

## Build Output

### Successful Build
```
[BUILD] Building server and client...
[BUILD] Build completed successfully!
[BUILD] Binaries generated:
  ✓ src/server/chat_server.exe (468K)
  ✓ src/client/chat_client.exe (278K)
```

### Server Output
```
Chat Server v2.0.0 listening on port 8080
```

## Environment Variables

The scripts automatically set:
- `PATH=/mingw64/bin:/usr/bin:/bin:$PATH` - MinGW compiler
- `PKG_CONFIG_PATH=/mingw64/lib/pkgconfig` - GTK4 detection

## Troubleshooting

### Build fails: "MSYS2 not found"
- Install MSYS2 from https://www.msys2.org/
- Default location: `C:\msys64`

### Build fails: "make: command not found"
- In MSYS2 terminal: `pacman -S make`

### Build fails: "gcc: command not found"  
- In MSYS2 terminal: `pacman -S mingw-w64-x86_64-gcc`

### Build fails: "gtk/gtk.h: No such file"
- In MSYS2 terminal: `pacman -S mingw-w64-x86_64-gtk4`

### Test server shows "Chat Server listening" but no client connection
- This is normal - server is waiting for connections
- Use Ctrl+C to stop

## File Structure

```
chat_program/
├── build.bat              # Build script (Command Prompt)
├── build.ps1              # Build script (PowerShell)
├── build.sh               # Build script (MSYS2 Bash)
├── test-server.bat        # Server test script
├── test-server.ps1        # Server test script (PowerShell)
├── Makefile               # Build system
├── src/
│   ├── server/
│   │   ├── chat_server.exe (generated)
│   │   └── *.c/*.h
│   ├── client/
│   │   ├── chat_client.exe (generated)
│   │   └── *.c/*.h
│   └── common/
│       └── *.c/*.h
└── sql/
    └── schema.sql
```

## Version

- **Server**: 2.0.0
- **Build Date**: 2026-05-04
- **Compiler**: gcc 15.2.0 (MinGW)
- **Language**: C11
