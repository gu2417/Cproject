# 비밀번호 해싱

## 사용 알고리즘: SHA-256

- 저장 위치: `users.txt`의 `pw_hash` 필드 (64자 hex 문자열)
- 사용 시점: 회원가입(저장), 로그인(비교)
- 평문 비밀번호는 메모리에서 해시 후 즉시 덮어쓰기

---

## WinCrypt API 구현

```c
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "Advapi32.lib")

/* pw → 64자 hex 문자열 sha256_out 으로 변환. 성공=1, 실패=0 */
int sha256_hex(const char *pw, char sha256_out[65]) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hash[32];
    DWORD hashLen = 32;
    int ok = 0;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return 0;
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        goto cleanup;
    if (!CryptHashData(hHash, (BYTE *)pw, (DWORD)strlen(pw), 0))
        goto cleanup;
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0))
        goto cleanup;

    for (int i = 0; i < 32; i++)
        sprintf(sha256_out + i * 2, "%02x", hash[i]);
    sha256_out[64] = '\0';
    ok = 1;

cleanup:
    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);
    return ok;
}
```

---

## 로그인 검증 흐름

```
1. 클라이언트: LOGIN_REQ|<id>:<plain_pw>
2. 서버: sha256_hex(plain_pw) → input_hash
3. 서버: users.txt에서 id 조회 → stored_hash
4. 비교: strcmp(input_hash, stored_hash) == 0 → 0=OK
5. 응답: LOGIN_RES|<code>
```

---

## 비밀번호 마스킹 (콘솔 입력)

```c
/* getch()로 한 자씩 읽어 '*' 에코, Enter(13) 시 종료 */
void read_password(char *buf, int max_len) {
    int i = 0;
    char ch;
    while (1) {
        ch = getch();
        if (ch == 13) {          /* Enter */
            buf[i] = '\0';
            printf("\n");
            break;
        } else if (ch == 8) {    /* Backspace */
            if (i > 0) { i--; printf("\b \b"); }
        } else if (i < max_len - 1) {
            buf[i++] = ch;
            printf("*");
        }
    }
}
```
