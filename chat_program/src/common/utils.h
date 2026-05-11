#pragma once

#include <stddef.h>

/* 현재 시간을 "YYYY-MM-DD HH:MM:SS" 형식으로 만든다. */
void get_current_timestamp(char out[20]);

/* 저장된 시간 문자열을 설정에 맞게 화면 출력용으로 바꾼다. */
void format_timestamp(const char *src, int fmt, char *out);

/* 메시지 안의 간단한 이모티콘 코드를 보기 쉬운 문자로 바꾼다. */
void convert_emoticons(const char *src, char *dst, size_t n);

/* 패킷 구분자로 쓰는 문자가 들어 있는지 확인한다. */
int has_forbidden_char(const char *s);

/* 메시지 안에 @닉네임 형태의 멘션이 있는지 확인한다. */
int detect_mention(const char *content, const char *nick);

/* 비밀번호 문자열을 SHA-256 해시 문자열로 바꾼다. */
void sha256_hex(const char *plain, char out[65]);

/* 문자열을 나누면서 다음 위치를 따로 보관한다. */
char *safe_strtok_r(char *str, const char *delim, char **saveptr);
