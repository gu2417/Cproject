#pragma once

#include "../common/types.h"

/* friend_init: router에 친구 패킷 핸들러를 등록한다. */
void friend_init(void);

/* receiver가 sender를 차단했는지 확인 */
int is_blocked_by(const char *receiver_id, const char *sender_id);
