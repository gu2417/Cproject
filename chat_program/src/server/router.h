#pragma once

#include "../common/types.h"

/* 패킷 핸들러 함수 포인터 타입 */
typedef void (*PacketHandler)(ClientSession *sess, char *payload);

/* 핸들러를 dispatch 테이블에 등록한다.
 * auth.c, room.c 등 각 모듈의 init 함수에서 호출한다. */
void register_handler(const char *type, PacketHandler fn);

/* line = "TYPE|PAYLOAD" 형태의 패킷 문자열을 파싱하여
 * 등록된 핸들러를 호출한다.
 * 알 수 없는 TYPE → send_packet(sess->sock, "ERROR|UNKNOWN_TYPE\n") */
void route_packet(ClientSession *sess, char *line);
