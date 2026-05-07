#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "../common/protocol.h"
#include "../common/types.h"
#include "../common/utils.h"
#include "globals.h"
#include "file_io.h"
#include "broadcast.h"
#include "router.h"
#include "message.h"

/* "//" 구분자로 line 을 최대 max_f 개 필드로 분리. 마지막 필드는 content-last.
 * 반환값: 파싱된 필드 수. line 은 파괴적으로 수정된다. */
static int split_fields(char *line, char **f, int max_f) {
    int len = (int)strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
        line[--len] = '\0';
    if (len == 0) return 0;
    int n = 0;
    char *p = line;
    char *sep;
    while (n < max_f - 1) {
        sep = strstr(p, "//");
        if (!sep) break;
        f[n++] = p;
        *sep = '\0';
        p = sep + 2;
    }
    f[n] = p;
    return n + 1;
}

/* "YYYY-MM-DD HH:MM:SS" 문자열을 time_t 로 변환 */
static time_t parse_timestamp(const char *ts) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    if (!ts || !*ts) return (time_t)0;
    sscanf(ts, "%d-%d-%d %d:%d:%d",
           &t.tm_year, &t.tm_mon, &t.tm_mday,
           &t.tm_hour, &t.tm_min, &t.tm_sec);
    t.tm_year -= 1900;
    t.tm_mon  -= 1;
    t.tm_isdst = -1;
    return mktime(&t);
}

/* messages.txt 에서 msg_id 레코드를 찾아 내용/편집시각을 갱신하고 파일을 재작성한다.
 * 성공 시 0, 권한 오류·5분 초과·미발견 시 음수 반환.
 * room_id_out 과 to_id_out 에 원본 메시지의 room_id/to_id 를 채운다.
 * MUTEX: 호출자가 g_file_mutex 를 보유해야 한다. */
static int do_msg_edit(int msg_id, const char *user_id,
                       const char *new_content, const char *edited_at,
                       int *room_id_out, char *to_id_out) {
    FILE *in = fopen(FILE_MESSAGES, "r");
    if (!in) return -1;

    FILE *out = fopen("data/msg_tmp.txt", "w");
    if (!out) { fclose(in); return -1; }

    /* messages.txt 포맷:
     * id//room_id//from_id//to_id//reply_to//msg_type//is_deleted//created_at//edited_at//content */
    char line[MAX_PKT_SIZE + 256];
    int  result = -1;

    while (fgets(line, sizeof(line), in)) {
        if (line[0] == '\n' || line[0] == '\r') { fputs(line, out); continue; }

        /* 첫 필드만 먼저 확인해 불필요한 파싱 생략 */
        if (atoi(line) != msg_id) { fputs(line, out); continue; }

        char tmp[MAX_PKT_SIZE + 256];
        strncpy(tmp, line, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        char *f[11];
        if (split_fields(tmp, f, 10) < 10) { fputs(line, out); continue; }

        /* 권한: 발신자 일치 */
        if (strcmp(f[2], user_id) != 0) { result = -2; fputs(line, out); continue; }
        /* 시스템 메시지 편집 불가 */
        if (atoi(f[5]) != MSG_TYPE_NORMAL) { result = -3; fputs(line, out); continue; }
        /* 삭제된 메시지 편집 불가 */
        if (atoi(f[6]) != 0) { result = -4; fputs(line, out); continue; }
        /* 5분(300초) 편집 창 확인 — created_at 기준 */
        if (difftime(time(NULL), parse_timestamp(f[7])) > 300.0) {
            result = -5; fputs(line, out); continue;
        }

        *room_id_out = atoi(f[1]);
        strncpy(to_id_out, f[3], 20);
        to_id_out[20] = '\0';

        /* 수정된 레코드 작성 (f[8]=edited_at, f[9]=content 갱신) */
        fprintf(out, "%s//%s//%s//%s//%s//%s//%s//%s//%s//%s\n",
                f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7],
                edited_at, new_content);
        result = 0;
    }
    fclose(in);
    fclose(out);

    if (result == 0) {
        remove(FILE_MESSAGES);
        rename("data/msg_tmp.txt", FILE_MESSAGES);
    } else {
        remove("data/msg_tmp.txt");
    }
    return result;
}

/* messages.txt 에서 msg_id 레코드를 is_deleted=1 로 표시하고 파일을 재작성한다.
 * MUTEX: 호출자가 g_file_mutex 를 보유해야 한다. */
static int do_msg_delete(int msg_id, const char *user_id,
                         int *room_id_out, char *to_id_out) {
    FILE *in = fopen(FILE_MESSAGES, "r");
    if (!in) return -1;

    FILE *out = fopen("data/msg_tmp.txt", "w");
    if (!out) { fclose(in); return -1; }

    char line[MAX_PKT_SIZE + 256];
    int  result = -1;

    while (fgets(line, sizeof(line), in)) {
        if (line[0] == '\n' || line[0] == '\r') { fputs(line, out); continue; }

        if (atoi(line) != msg_id) { fputs(line, out); continue; }

        char tmp[MAX_PKT_SIZE + 256];
        strncpy(tmp, line, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        char *f[11];
        if (split_fields(tmp, f, 10) < 10) { fputs(line, out); continue; }

        if (strcmp(f[2], user_id) != 0) { result = -2; fputs(line, out); continue; }
        if (atoi(f[6]) != 0) { result = -3; fputs(line, out); continue; } /* 이미 삭제됨 */

        *room_id_out = atoi(f[1]);
        strncpy(to_id_out, f[3], 20);
        to_id_out[20] = '\0';

        /* is_deleted=1 로 재작성 (f[6] 자리에 1 삽입) */
        fprintf(out, "%s//%s//%s//%s//%s//%s//1//%s//%s//%s\n",
                f[0], f[1], f[2], f[3], f[4], f[5], f[7], f[8], f[9]);
        result = 0;
    }
    fclose(in);
    fclose(out);

    if (result == 0) {
        remove(FILE_MESSAGES);
        rename("data/msg_tmp.txt", FILE_MESSAGES);
    } else {
        remove("data/msg_tmp.txt");
    }
    return result;
}

/* MSG_EDIT|room_id:msg_id:new_content  (new_content 은 content-last)
 * 5분 창 체크 → MSG_EDITED_NOTIFY|room_id:msg_id:new_content 브로드캐스트 */
static void handle_msg_edit(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *msg_id_s  = strtok(NULL,    ":");
    char *new_cont  = strtok(NULL,    "");
    if (!room_id_s || !msg_id_s || !new_cont) return;

    int n = (int)strlen(new_cont);
    while (n > 0 && (new_cont[n-1] == '\r' || new_cont[n-1] == '\n'))
        new_cont[--n] = '\0';
    if (n == 0) return;

    int msg_id = atoi(msg_id_s);

    char edited_at[20];
    get_current_timestamp(edited_at);

    int  to_room_id = atoi(room_id_s);
    char to_id[21]  = {0};

    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    int ret = do_msg_edit(msg_id, sess->user_id, new_cont, edited_at,
                          &to_room_id, to_id);
    ReleaseMutex(g_file_mutex);

    if (ret != 0) return;

    char buf[MAX_PKT_SIZE];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         MSG_EDITED_NOTIFY "|%d:%d:%s",
                         to_room_id, msg_id, new_cont);
    if (mlen <= 0 || mlen >= (int)sizeof(buf) - 2) return;
    buf[mlen++] = '\n'; buf[mlen] = '\0';

    if (to_room_id == 0) {
        /* DM: 두 당사자에게만 전송 */
        send_to_user(sess->user_id, buf);
        if (to_id[0]) send_to_user(to_id, buf);
    } else {
        broadcast_to_room(to_room_id, buf);
    }
}

/* MSG_DELETE|room_id:msg_id
 * is_deleted=1 로 표시 → MSG_DELETED_NOTIFY|room_id:msg_id 전송
 * DM(room_id=0)은 두 당사자에게만, 그룹은 방 브로드캐스트 */
static void handle_msg_delete(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *msg_id_s  = strtok(NULL,    "");
    if (!room_id_s || !msg_id_s) return;

    int n = (int)strlen(msg_id_s);
    while (n > 0 && (msg_id_s[n-1] == '\r' || msg_id_s[n-1] == '\n'))
        msg_id_s[--n] = '\0';

    int msg_id = atoi(msg_id_s);

    int  to_room_id = atoi(room_id_s);
    char to_id[21]  = {0};

    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    int ret = do_msg_delete(msg_id, sess->user_id, &to_room_id, to_id);
    ReleaseMutex(g_file_mutex);

    if (ret != 0) return;

    char buf[256];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         MSG_DELETED_NOTIFY "|%d:%d",
                         to_room_id, msg_id);
    if (mlen <= 0 || mlen >= (int)sizeof(buf) - 2) return;
    buf[mlen++] = '\n'; buf[mlen] = '\0';

    if (to_room_id == 0) {
        send_to_user(sess->user_id, buf);
        if (to_id[0]) send_to_user(to_id, buf);
    } else {
        broadcast_to_room(to_room_id, buf);
    }
}

/* MSG_REPLY|room_id:reply_to_id:content (P3 — 미구현) */
static void handle_msg_reply(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;
}

/* MSG_SEARCH|room_id:keyword (P3 — 미구현) */
static void handle_msg_search(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;
    send_packet(sess->sock, MSG_SEARCH_RES "|0");
}

void message_init(void) {
    register_handler(MSG_EDIT,   handle_msg_edit);
    register_handler(MSG_DELETE, handle_msg_delete);
    register_handler(MSG_REPLY,  handle_msg_reply);
    register_handler(MSG_SEARCH, handle_msg_search);
}
