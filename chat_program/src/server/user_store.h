#pragma once

#include "../common/types.h"

/* user_store_init: router에 유저/설정 패킷 핸들러를 등록한다. */
void user_store_init(void);

/* 유저 닉네임 반환 (없으면 user_id 복사) */
void get_nickname(const char *user_id, char out_nick[21]);

/* 마지막 접속 시간을 현재 시각으로 갱신 (g_file_mutex 내부 획득) */
void update_last_seen(const char *user_id);

/* 친구에게 온라인 상태 변경 알림 전송 */
void notify_friend_status_change(const char *user_id, int new_status);
