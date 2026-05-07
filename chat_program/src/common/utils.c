#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "utils.h"

void get_current_timestamp(char out[20]) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(out, 20, "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

void format_timestamp(const char *src, int fmt, char *out) {
    /* src: "YYYY-MM-DD HH:MM:SS" */
    int year, mon, day, hour, min, sec;
    if (sscanf(src, "%d-%d-%d %d:%d:%d",
               &year, &mon, &day, &hour, &min, &sec) < 6) {
        strncpy(out, src, 19);
        out[19] = '\0';
        return;
    }
    switch (fmt) {
        case 1:
            snprintf(out, 10, "%02d:%02d:%02d", hour, min, sec);
            break;
        case 2:
            snprintf(out, 12, "%02d-%02d %02d:%02d", mon, day, hour, min);
            break;
        case 0:
        default:
            snprintf(out, 7, "%02d:%02d", hour, min);
            break;
    }
}

void convert_emoticons(const char *src, char *dst, size_t n) {
    static const struct { const char *code; const char *repl; } table[] = {
        { ":smile:", "(^_^)" },
        { ":sad:",   "(T_T)" },
        { ":heart:", "(\xe2\x99\xa5)" },  /* UTF-8 ♥ */
        { ":laugh:", "(\xed\x81\xac\xed\x81\xac)" }, /* ㅋㅋ */
        { ":angry:", "(>_<)" },
        { NULL, NULL }
    };

    size_t di = 0;
    size_t si = 0;
    size_t slen = strlen(src);

    while (si < slen && di + 1 < n) {
        int matched = 0;
        for (int i = 0; table[i].code; i++) {
            size_t clen = strlen(table[i].code);
            if (strncmp(src + si, table[i].code, clen) == 0) {
                size_t rlen = strlen(table[i].repl);
                if (di + rlen < n) {
                    memcpy(dst + di, table[i].repl, rlen);
                    di += rlen;
                }
                si += clen;
                matched = 1;
                break;
            }
        }
        if (!matched) {
            dst[di++] = src[si++];
        }
    }
    dst[di] = '\0';
}

int has_forbidden_char(const char *s) {
    for (; *s; s++) {
        if (*s == ':' || *s == ';' || *s == '|' || *s == '\n' || *s == '\r')
            return 1;
    }
    return 0;
}

int detect_mention(const char *content, const char *nick) {
    if (!content || !nick || !*nick) return 0;
    size_t nlen = strlen(nick);
    const char *p = content;
    while ((p = strstr(p, "@")) != NULL) {
        /* @ 앞이 단어 경계인지 확인 */
        if (p > content && (isalnum((unsigned char)*(p-1)) || *(p-1) == '_'))
            { p++; continue; }
        p++; /* @ 다음 */
        if (strncmp(p, nick, nlen) == 0) {
            /* nick 뒤가 단어 경계인지 확인 */
            char next = *(p + nlen);
            if (!isalnum((unsigned char)next) && next != '_')
                return 1;
        }
    }
    return 0;
}

void sha256_hex(const char *plain, char out[65]) {
    out[0] = '\0';
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE  hash[32];
    DWORD hashLen = 32;

    if (!CryptAcquireContextA(&hProv, NULL, NULL,
                              PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        fprintf(stderr, "[sha256] CryptAcquireContext failed: %lu\n",
                GetLastError());
        return;
    }
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        fprintf(stderr, "[sha256] CryptCreateHash failed: %lu\n",
                GetLastError());
        CryptReleaseContext(hProv, 0);
        return;
    }
    if (!CryptHashData(hHash, (const BYTE*)plain, (DWORD)strlen(plain), 0)) {
        fprintf(stderr, "[sha256] CryptHashData failed: %lu\n",
                GetLastError());
        goto cleanup;
    }
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        fprintf(stderr, "[sha256] CryptGetHashParam failed: %lu\n",
                GetLastError());
        goto cleanup;
    }
    for (DWORD i = 0; i < hashLen; i++)
        snprintf(out + i * 2, 3, "%02x", hash[i]);
    out[64] = '\0';

cleanup:
    if (hHash) CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}

char *safe_strtok_r(char *str, const char *delim, char **saveptr) {
    char *start;
    if (str != NULL)
        *saveptr = str;
    start = *saveptr;
    if (!start || !*start)
        return NULL;
    /* 구분자 건너뜀 */
    start += strspn(start, delim);
    if (!*start) {
        *saveptr = start;
        return NULL;
    }
    /* 토큰 끝 찾기 */
    char *end = start + strcspn(start, delim);
    if (*end) {
        *end = '\0';
        *saveptr = end + 1;
    } else {
        *saveptr = end;
    }
    return start;
}
