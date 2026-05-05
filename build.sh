#!/bin/bash
# Chat Program Build Script for MSYS2 Bash
# Usage: ./build.sh [clean] [verbose]

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Helper functions
log_status() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

# Parse arguments
CLEAN=0
VERBOSE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        clean)
            CLEAN=1
            shift
            ;;
        verbose)
            VERBOSE=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [clean] [verbose]"
            exit 1
            ;;
    esac
done

# Configuration
export PATH=/mingw64/bin:/usr/bin:/bin:$PATH
export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig

# Check prerequisites
log_status "Checking prerequisites..."

if ! command -v gcc &> /dev/null; then
    log_error "gcc not found. Please install MinGW via MSYS2."
    exit 1
fi

if ! command -v make &> /dev/null; then
    log_error "make not found. Please run: pacman -S make"
    exit 1
fi

GCC_VERSION=$(gcc --version | head -1)
log_info "Using: $GCC_VERSION"

# Change to project directory
cd "$(dirname "$0")" || exit 1

if [ $CLEAN -eq 1 ]; then
    log_status "Cleaning build artifacts..."
    make UNAME="$(uname -s)" clean || true
fi

# Build
log_status "Starting build..."
log_info "Command: make UNAME=$(uname -s)"
echo ""

if [ $VERBOSE -eq 1 ]; then
    make UNAME="$(uname -s)" 2>&1
else
    make UNAME="$(uname -s)" 2>&1 | grep -E '^\[|error|Error|ERROR' || true
fi

BUILD_EXIT=$?

# Check results
echo ""
if [ $BUILD_EXIT -eq 0 ]; then
    log_status "Build completed successfully!"
    
    # Check binaries
    if [ -f "src/server/chat_server.exe" ] && [ -f "src/client/chat_client.exe" ]; then
        log_status "Binaries generated:"
        ls -lh src/server/chat_server.exe src/client/chat_client.exe | awk '{print "  ✓ " $9 " (" $5 ")"}'
    fi
else
    log_error "Build failed with exit code $BUILD_EXIT"
    exit $BUILD_EXIT
fi

echo ""
log_status "Build script completed."
echo ""
log_info "Next steps:"
echo "  1. Test server: timeout 5 ./src/server/chat_server.exe"
echo "  2. Run client: ./src/client/chat_client.exe"
