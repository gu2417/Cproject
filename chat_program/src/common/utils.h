#pragma once

#include <stddef.h>

/* 현재 시각을 "YYYY-MM-DD HH:MM:SS" 형식으로 out에 쓴다 (out은 최소 20바이트) */
void get_current_timestamp(char out[20]);

/* src 타임스탬프를 fmt에 따라 변환
 * fmt 0: "HH:MM"
 * fmt 1: "HH:MM:SS"
 * fmt 2: "MM-DD HH:MM"
 */
void format_timestamp(const char *src, int fmt, char *out);

/* src의 이모티콘 코드를 변환하여 dst에 쓴다 (n = dst 버퍼 크기)
 * :smile: → (^_^)
 * :sad:   → (T_T)
 * :heart: → (♥)
 * :laugh: → (ㅋㅋ)
 * :angry: → (>_<)
 */
void convert_emoticons(const char *src, char *dst, size_t n);

/* s에 금지문자(: ; | \n)가 있으면 1, 없으면 0 반환 */
int has_forbidden_char(const char *s);

/* content에서 @nick 패턴이 워드 경계에 있으면 1, 없으면 0 반환 */
int detect_mention(const char *content, const char *nick);

/* plain을 SHA-256 해시하여 hex-lowercase 64자+\0을 out에 쓴다
 * 실패 시 out[0]='\0' 보장
 */
void sha256_hex(const char *plain, char out[65]);

/* 재진입 가능한 strtok 래퍼 */
char *safe_strtok_r(char *str, const char *delim, char **saveptr);
