#pragma once

void InitialMenu(void);

/* 비밀번호 입력 (getch 마스킹, * 표시) */
void read_password(char *buf, int maxlen);
