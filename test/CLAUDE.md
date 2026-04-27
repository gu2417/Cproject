# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A console-based real-time chat application in C (C11) modeled after KakaoTalk/Google Chat. TCP server-client architecture with multi-threading. Target: Linux and Windows (MinGW) dual builds.

## Build Commands

```bash
# Build everything
make

# Build server only
make server

# Build client only
make client

# Clean
make clean
```

Expected binaries: `chat_program/src/server/chat_server`, `chat_program/src/client/chat_client`

## Running

```bash
# Start server (default port 8080)
./chat_program/src/server/chat_server [port]

# Start client
./chat_program/src/client/chat_client [host] [port]
```

## Architecture

Three-layer layout under `chat_program/src/`: `server/`, `client/`, `common/`

**Common layer** (`chat_program/src/common/`) defines the contract for both sides:
- `protocol.h` — all packet type constants and delimiters (`|` field sep, `:` multi-value sep, `\n` terminator, max 1024 bytes/packet)
- `types.h` — shared structs: `User`, `FriendEntry`, `ChatRoom`, `Message`
- `utils.c/h` — timestamp formatting, string helpers

**Server** (`chat_program/src/server/`) is a thread-per-client model:
- `main.c` — accept loop, spawns a handler thread per connection
- `client_handler.c` — reads packets from socket, calls router
- `router.c` — dispatches packet type to the appropriate handler (auth, friend, room, dm, message, admin)
- `broadcast.c` — fan-out to room members and notification delivery
- All state is in-memory arrays (users[], rooms[], messages[], friends[])

**Client** (`chat_program/src/client/`) is a two-thread model (send + receive) with a TUI renderer:
- `net.c` — socket connect, send thread (stdin → socket), receive thread (socket → display)
- `ui.c` — console TUI layout with tab-style screens
- `input.c` — slash command parser
- `screen_login.c`, `screen_main.c`, `screen_chat.c` — individual screen renderers
- `notify.c` — notification banner overlay

## Packet Protocol

All packets follow `<TYPE>|<PAYLOAD>\n`. Multi-value payloads use `:` as separator within a field and `;` between list entries. See `requirements.md` §4 for the full packet definition table.

## Key Data Structures (from `chat_program/src/common/types.h`)

- `User` — id[21], pass_hash[65], nickname[21], socket_fd (-1 if offline), dnd flag
- `ChatRoom` — room_id, owner_id, member_fds[64] (socket fd list of current members)
- `Message` — msg_id, room_id (0 for DM), from_id, to_id, content[501], is_deleted, read_count
- `FriendEntry` — status: 0=pending, 1=accepted, 2=blocked

## Development Priorities

- **P0**: Socket connection, login/register, group chat broadcast
- **P1**: DM, friend management, read receipts
- **P2**: Whisper (`/w`), message delete, room notices
- **P3**: Search, emoticons, admin commands, DND mode

## Platform Notes

- Use `pthread` on Linux; `_beginthreadex` or equivalent on Windows/MinGW
- Socket headers: `<sys/socket.h>` (Linux) vs `<winsock2.h>` (Windows) — guard with `#ifdef _WIN32`
- Password storage: one-way hash only (no plaintext) — see NFR-04 in `requirements.md`
