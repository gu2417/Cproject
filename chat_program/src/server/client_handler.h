#pragma once

#include <winsock2.h>
#include <windows.h>
#include "../common/types.h"

/* _beginthreadex 엔트리 포인트.
 * arg = (void*)(intptr_t)slot — g_sessions[] 의 슬롯 인덱스 */
/* 클라이언트 한 명의 수신 반복 처리를 맡는다. */
unsigned WINAPI HandleClient(void *arg);

/* 세션을 정리하고 소켓을 닫는다.
 * 1. 로그인 상태이고 방에 있으면 시스템 퇴장 메시지 broadcast_to_room
 * 2. g_sessions 슬롯 초기화 (active=0, user_id 클리어, closesocket)
 * MUTEX: g_sessions_mutex 를 내부에서 획득·해제 */
/* 클라이언트가 나갈 때 세션과 소켓을 정리한다. */
void leftClient(ClientSession *sess);
