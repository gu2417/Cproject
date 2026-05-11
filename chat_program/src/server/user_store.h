#pragma once

#include "../common/types.h"

/* user_store_init: router에 유저/설정 패킷 핸들러를 등록한다. */
/* 마이페이지와 설정 관련 요청 처리 함수를 등록한다. */
void user_store_init(void);

/* 유저 닉네임 반환 (없으면 user_id 복사) */
/* 아이디에 해당하는 닉네임을 찾아 out_nick에 넣는다. */
void get_nickname(const char *user_id, char out_nick[21]);

/* 마지막 접속 시간을 현재 시각으로 갱신 (g_file_mutex 내부 획득) */
/* 사용자의 마지막 접속 시간을 현재 시간으로 바꾼다. */
void update_last_seen(const char *user_id);

/* 친구에게 온라인 상태 변경 알림 전송 */
/* 친구들에게 사용자의 온라인 상태 변경을 알려 준다. */
void notify_friend_status_change(const char *user_id, int new_status);
