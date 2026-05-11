#pragma once

#include "../common/types.h"

/* auth_init: router에 인증 패킷 핸들러를 등록한다. */
/* 로그인, 회원가입, 로그아웃 같은 계정 관련 요청을 등록한다. */
void auth_init(void);
