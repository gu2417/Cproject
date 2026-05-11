#pragma once

#include "../common/types.h"

/* 패킷 핸들러 함수 포인터 타입 */
/* 패킷을 처리하는 함수의 모양을 정해 둔다. */
typedef void (*PacketHandler)(ClientSession *sess, char *payload);

/* 핸들러를 dispatch 테이블에 등록한다.
 * auth.c, room.c 등 각 모듈의 init 함수에서 호출한다. */
/* 패킷 종류와 처리 함수를 라우터에 등록한다. */
void register_handler(const char *type, PacketHandler fn);

/* line = "TYPE|PAYLOAD" 형태의 패킷 문자열을 파싱하여
 * 등록된 핸들러를 호출한다.
 * 알 수 없는 TYPE → send_packet(sess->sock, "ERROR|UNKNOWN_TYPE\n") */
/* 받은 문자열을 패킷 종류와 내용으로 나누어 알맞은 함수로 넘긴다. */
void route_packet(ClientSession *sess, char *line);
