#pragma once

#include "../common/types.h"

/* friend_init: router에 친구 패킷 핸들러를 등록한다. */
/* 친구 관련 요청 처리 함수를 등록한다. */
void friend_init(void);

/* receiver가 sender를 차단했는지 확인 */
/* receiver_id가 sender_id를 차단했는지 확인한다. */
int is_blocked_by(const char *receiver_id, const char *sender_id);

/* 사용자가 상대를 차단 상태로 바꾼다. */
int friend_block_user(const char *user_id, const char *target_id);

/* 사용자가 상대의 차단을 해제한다. */
int friend_unblock_user(const char *user_id, const char *target_id);
