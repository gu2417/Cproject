---
name: packet-add
description: Add a new packet TYPE to protocol.h, packet_reference.md, and router dispatch table simultaneously.
when_to_use: Use when adding a new packet type to avoid missing any of the 3 locations.
---
# Packet Add Skill

실행: `/packet-add <TYPE>`

다음 3곳을 동시 편집:
1. `chat_program/src/common/protocol.h` — `#define <TYPE> "<TYPE>"` 추가
2. `docs/protocol/packet_reference.md` — 패킷 설명 항목 추가
3. `chat_program/src/server/router.c` — dispatch 테이블에 핸들러 추가
