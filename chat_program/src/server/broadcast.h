#pragma once

#include <winsock2.h>
#include "../common/types.h"

/* 단일 소켓에 printf 스타일로 패킷을 전송한다.
 * 포맷 문자열에 \n 이 없으면 자동으로 붙인다. */
void send_packet(SOCKET sock, const char *fmt, ...);

/* P0: 로그인된 모든 활성 세션에 msg 를 송신한다. */
void broadcast_to_all(const char *msg);

/* P1: 특정 room_id 에 속한 세션에만 msg 를 송신한다. */
void broadcast_to_room(int room_id, const char *msg);

/* 특정 user_id 의 온라인 세션에 msg 를 송신한다. */
void send_to_user(const char *user_id, const char *msg);
