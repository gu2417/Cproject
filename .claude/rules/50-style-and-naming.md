# Code Style and Naming Rules

## 헤더 include 순서
```c
#include <winsock2.h>   // 반드시 첫 번째
#include <windows.h>    // 반드시 두 번째
#include <process.h>    // _beginthreadex
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// 프로젝트 헤더
#include "../common/types.h"
#include "../common/protocol.h"
```

## 네이밍 규칙
- 함수: `snake_case`
- 핸들러: `handle_<verb>` (예: `handle_login`, `handle_room_create`)
- 전역 상수: `MAX_*`, `FILE_*`, `DEFAULT_*`
- 전역 배열: `g_` prefix (예: `g_sessions`, `g_rooms`)

## 들여쓰기
- 4 spaces (탭 금지)

## 코멘트
- mutex 보호: `// MUTEX: g_xxx`
- plan-originated 결정: `// PLAN: <설명>`
- docs errata: `// ERRATA: <설명>`
