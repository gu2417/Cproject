#pragma once

#include "../common/types.h"

/* room_init: router에 채팅방 패킷 핸들러를 등록한다. */
/* 채팅방 관련 요청 처리 함수를 등록한다. */
void room_init(void);

/* 방 멤버인지 확인 (g_rooms 직접 검색) */
/* 사용자가 해당 방의 멤버인지 확인한다. */
int is_room_member(int room_id, const char *user_id);
