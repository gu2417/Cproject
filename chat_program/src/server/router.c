#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/types.h"
#include "globals.h"
#include "broadcast.h"
#include "router.h"

/* ---------------------------------------------------------------
 * 등록 기반 dispatch 테이블.
 * 각 모듈(auth.c, room.c ...)의 init 함수가 register_handler() 를 호출한다.
 * --------------------------------------------------------------- */
#define MAX_HANDLERS 64

typedef struct {
    char          type[32];
    PacketHandler fn;
} HandlerEntry;

static HandlerEntry s_handlers[MAX_HANDLERS];
static int          s_handler_count = 0;

/* ---------------------------------------------------------------
 * register_handler: 패킷 타입과 핸들러 함수를 등록한다.
 * --------------------------------------------------------------- */
/* 패킷 종류와 처리 함수를 등록한다. */
void register_handler(const char *type, PacketHandler fn) {
    if (s_handler_count >= MAX_HANDLERS) {
        fprintf(stderr, "[router] 핸들러 테이블 포화: %s 등록 실패\n", type);
        return;
    }
    strncpy(s_handlers[s_handler_count].type, type, 31);
    s_handlers[s_handler_count].type[31] = '\0';
    s_handlers[s_handler_count].fn       = fn;
    s_handler_count++;
}

/* ---------------------------------------------------------------
 * route_packet: "TYPE|PAYLOAD" 형태의 라인을 파싱하여 핸들러를 호출한다.
 * line 은 수정 가능한 버퍼여야 한다 (strtok 사용).
 * --------------------------------------------------------------- */
/* 받은 패킷 문자열을 나누고 맞는 처리 함수를 부른다. */
void route_packet(ClientSession *sess, char *line) {
    char  tmp[MAX_PKT_SIZE];
    char *type;
    char *payload;
    int   i;

    strncpy(tmp, line, MAX_PKT_SIZE - 1);
    tmp[MAX_PKT_SIZE - 1] = '\0';

    type    = strtok(tmp, "|");
    payload = strtok(NULL, "");   /* content-last: 나머지 전부 */

    if (!type || type[0] == '\0') return;

    for (i = 0; i < s_handler_count; i++) {
        if (strcmp(s_handlers[i].type, type) == 0) {
            s_handlers[i].fn(sess, payload ? payload : "");
            return;
        }
    }

    send_packet(sess->sock, "ERROR|UNKNOWN_TYPE\n");
}
