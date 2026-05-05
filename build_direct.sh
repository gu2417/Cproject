#!/bin/bash
# Direct build script for chat program

set -e

export PATH=/mingw64/bin:/usr/bin:/bin:$PATH

cd "$(dirname "$0")"

echo "[BUILD] Cleaning..."
rm -f src/server/chat_server src/client/chat_client
rm -f src/server/*.o src/client/*.o src/common/*.o

echo "[BUILD] Compiling server..."
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc/common \
    -o src/server/chat_server \
    src/server/auth.c \
    src/server/broadcast.c \
    src/server/client_handler.c \
    src/server/db.c \
    src/server/dm.c \
    src/server/friend.c \
    src/server/globals.c \
    src/server/main.c \
    src/server/message.c \
    src/server/room.c \
    src/server/router.c \
    src/server/user_store.c \
    src/common/net_win.c \
    src/common/utils.c \
    -lpthread -lws2_32

echo "[BUILD] Server built successfully: src/server/chat_server"
ls -lh src/server/chat_server

echo "[BUILD] Done!"
