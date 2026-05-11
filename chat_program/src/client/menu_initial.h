#pragma once

/* 처음 실행했을 때 보이는 로그인/회원가입 메뉴를 출력한다. */
void InitialMenu(void);

/* 비밀번호 입력 (getch 마스킹, * 표시) */
/* 비밀번호를 화면에 별표로 보이게 하면서 입력받는다. */
void read_password(char *buf, int maxlen);
