# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Status

This repo is currently in **design/documentation phase** — there is no `src/`, `Makefile`, or `data/` directory yet. The work product so far is `requirements.md` (original spec) and a comprehensive `docs/` tree that defines the implementation plan. Reference C implementations from a prior project exist in `reference/` (read-only inputs).

When implementing code: follow `docs/` over `requirements.md` where they conflict. The docs intentionally diverge from the spec on two big decisions (see "Key Design Decisions" below). The reconciliation is recorded in `docs/overview/requirements_traceability.md` — consult it before starting any non-trivial change.

## Project: C Console Chat Application

Real-time chat app modeled after KakaoTalk / Google Chat Spaces:
- **Language**: C11
- **Platform**: Windows only (WinSock2, `_beginthreadex`, `CreateMutex`)
- **Architecture**: TCP server-client, multi-threaded, console UI, text-file persistence
- **Default port**: 55555 (`DEFAULT_PORT`)
- **Limits**: `MAX_CLIENTS=256`, `MAX_ROOMS=100`, `MAX_ROOM_MEMBERS=64`, `MAX_PKT_SIZE=10240`, `MAX_MSG_HISTORY=1000`

## Key Design Decisions (do not revert)

1. **Console UI, not GTK4.** `requirements.md` originally specified GTK4 GUI with `screen_*.c/h` and CSS files. Docs replaced this with `client/menu_*.c/h` console menus using `printf`/`scanf`/`getch`. Do **not** add GTK4 dependencies.
2. **Text files, not MySQL.** `requirements.md` originally had MySQL DDL. Docs replaced with 9 `data/*.txt` files using `//` as field separator. Do **not** introduce a database driver. The `MYSQL *db` field in `ClientSession` was removed; `server/db.c/h` was replaced with `server/file_io.c/h`.
3. **`SOCKET` not `int`.** `ClientSession.fd` is `SOCKET` (WinSock2 `UINT_PTR`), not `int`. Compare with `INVALID_SOCKET`, not `< 0`.
4. **`server/admin.c/h` was deleted.** `FR-ADM01~05` are Out-of-Scope.
5. **`common/net_win.c` was deleted.** Folded into `client/net.c`. No platform abstraction layer.

## Source-of-Truth Hierarchy

When two docs disagree, trust this order:

1. `docs/overview/requirements_traceability.md` — FR/NFR/packet ↔ docs cross-reference matrix
2. `docs/database/in_memory_structures.md` — struct definitions, globals, mutexes, all `MAX_*` and `FILE_*` constants
3. `docs/protocol/packet_reference.md` — all ~65 packet TYPEs and field layouts
4. `docs/database/file_schema.md` — exact `data/*.txt` field order and `//` delimiter
5. `docs/architecture/module_*.md` — per-module C function signatures and reference implementations
6. `docs/features/FR_*.md` — feature-level acceptance criteria and policy matrices (block, edit/delete permissions)
7. `requirements.md` — original spec (authoritative for FR semantics, not for stack/structure)

## Critical Conventions

**Packet format** (`packet_format.md`):
```
<TYPE>|<PAYLOAD>\n
```
- Field separator inside payload: `:`
- List item separator: `;`
- **Content-last rule**: free-text fields (`content`, `status_msg`, `topic`, `notice`, `keyword`) must be the last field — parser uses `strtok(NULL, "")` to grab the rest verbatim.
- **List packets with free text** (`FRIEND_LIST_RES`, `USER_SEARCH_RES`, `DM_HISTORY_RES`, `ROOM_HISTORY_RES`, `MSG_SEARCH_RES`, `DM_LIST_RES`) must include a `<count>` first field; parser loops `count` times.
- **DM identifier**: `room_id == 0` means DM. `MSG_DELETE|0:<msg_id>` etc.

**Forbidden chars in user input** (`:` `;` `|` `\n`): reject at registration/input time. ID/nickname/password/room name/room password all enforce this. See `security/input_validation.md`.

**Text file delimiter**: `//` between fields. Empty field between two `//` produces `////` (4 slashes). A common bug pattern is writing `/////` (5 slashes), which puts a literal `/` as the field value — recheck `fprintf` format strings carefully.

**Three mutexes** (`in_memory_structures.md` §6):
- `g_sessions_mutex` — `g_sessions[]`, `g_rooms[]`, session counts
- `g_file_mutex` — every `fopen`/`fprintf` call against `data/*.txt`
- `g_console_mutex` — `RecvMsg` thread vs main thread console output

**Thread model**:
- Server: 1 accept loop + N `HandleClient` threads (per-client) + dispatch via `router.c`
- Client: 1 main menu loop + 1 `RecvMsg` thread for async server pushes

## Phase-Based Implementation Order

Implement in `development_phases.md` order:
- **P0**: FR-A01~A03, FR-G01/G03/G05, FR-O01/O02/O04, ROOM_JOIN. Brodcast is `broadcast_to_all()` (clients filter by `room_id`).
- **P1**: friends, DM, invites, history, mypage, `LOGOUT_REQ`, `ALREADY_ONLINE`. Switch to `broadcast_to_room()`.
- **P2**: msg edit/delete (5min), admin, customization, `PING/PONG`.
- **P3**: reply, search, pin, /me, typing, DND, mention.
- **Out-of-Scope**: FR-M05 (reactions), FR-ADM01~05.

## Build (planned, not yet wired up)

From `docs/build/build_guide.md`. The Makefile and source files don't exist yet, but when implementing:

```bash
# Server (MinGW)
gcc -std=c11 -Wall -Wextra -o server.exe \
    src/server/*.c src/common/utils.c \
    -lws2_32 -ladvapi32

# Client (MinGW)
gcc -std=c11 -Wall -Wextra -o client.exe \
    src/client/*.c src/common/utils.c \
    -lws2_32

# Initialize data/ (one-time)
mkdir data
type nul > data\users.txt   # repeat for all 9 txt files listed in file_schema.md
```

`-ladvapi32` is required for SHA-256 via WinCrypt (`security/password_hashing.md`).

## Common Pitfalls Specific to This Codebase

- **Never call `feof(fp)` without NULL-checking `fp` first** — `data/*.txt` may not exist on first run; `fopen` returns NULL and the spec requires graceful empty-array startup.
- **`recv()` returning 0 or negative** must call `leftClient()` to broadcast leave message + clear session slot + close socket. Don't just `break`.
- **Open-chat nickname (`open_nick`)**: when building `ROOM_HISTORY_RES`, `ROOM_MSG_RECV`, `ROOM_PIN`, or `MSG_SEARCH_RES` for an `is_open=1` room, look up the user's `open_nick` in `room_members.txt` first; fall back to `users.txt nickname` only if empty. The helper is `resolve_display_nick(room_id, user_id, is_open)` in `module_room.md`.
- **Block check direction**: `is_blocked_by(receiver_id, sender_id)` — pass receiver first. Used in DM, friend requests, room invites, whispers. See block matrix in `features/FR_F_friend.md`.
- **Message edit window is 5 minutes**, calculated from `created_at` not `edited_at`. System messages (`msg_type=1`) are never editable, even by room owner.
- **Single password hash** — both `users.txt pw_hash` and `rooms.txt pw_hash` are SHA-256 hex-lowercase, 64 chars. The plaintext limit is 19 (user) / 10 (room) — verify before hashing.

## Reference Implementation Note

`reference/server_main.c` and `reference/client_main.c` are the prior art that justified the `//`-delimited text file format and the console-based design. They are smaller in scope than the planned implementation but show the WinSock2 + per-thread + `users.txt` pattern in concrete form.
