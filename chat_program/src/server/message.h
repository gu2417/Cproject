#pragma once

#include "../common/types.h"

/* message_init: router에 메시지 패킷 핸들러를 등록한다. */
/* 메시지 수정, 삭제, 검색 같은 요청 처리 함수를 등록한다. */
void message_init(void);
