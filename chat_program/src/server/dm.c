#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/protocol.h"
#include "../common/types.h"
#include "../common/utils.h"
#include "globals.h"
#include "file_io.h"
#include "broadcast.h"
#include "router.h"
#include "user_store.h"
#include "friend.h"
#include "dm.h"

/* DM_SEND|to_id:content (content-last)
 * → DM_RECV|from_id:from_nick:HH.MM:msg_id:content 를 대상에게 전송 */
static void handle_dm_send(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *to_id  = strtok(payload, ":");
    char *content = strtok(NULL, "");
    if (!to_id || !content) return;

    int n = (int)strlen(content);
    while (n > 0 && (content[n-1] == '\r' || content[n-1] == '\n'))
        content[--n] = '\0';
    if (n == 0) return;

    n = (int)strlen(to_id);
    while (n > 0 && (to_id[n-1] == '\r' || to_id[n-1] == '\n'))
        to_id[--n] = '\0';

    if (!find_user_by_id(to_id)) return;
    if (is_blocked_by(to_id, sess->user_id)) return;

    char nick[21], ts_long[20], ts_short[6];
    get_nickname(sess->user_id, nick);
    get_current_timestamp(ts_long);

    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        snprintf(ts_short, 6, "%02d.%02d", st.wHour, st.wMinute);
    }

    int msg_id;
    WaitForSingleObject(g_file_mutex, INFINITE);
    msg_id = g_next_msg_id++;
    {
        MessageRecord m;
        memset(&m, 0, sizeof(m));
        m.id      = msg_id;
        m.room_id = 0;              /* DM: room_id == 0 */
        strncpy(m.from_id,   sess->user_id, 20);
        strncpy(m.from_nick, nick,          20);
        strncpy(m.to_id,     to_id,         20);
        m.msg_type = MSG_TYPE_NORMAL;
        strncpy(m.created_at, ts_long, 19);
        strncpy(m.content, content, MAX_PKT_SIZE - 1);
        append_message(FILE_MESSAGES, &m);
    }
    ReleaseMutex(g_file_mutex);

    char buf[MAX_PKT_SIZE];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         DM_RECV "|%s:%s:%s:%d:%s",
                         sess->user_id, nick, ts_short, msg_id, content);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        send_to_user(to_id, buf);
        /* 자기 자신에게 보내는 DM이 아니면 송신자에게도 echo */
        if (strcmp(sess->user_id, to_id) != 0)
            send_to_user(sess->user_id, buf);
    }
}

/* "//" 구분자로 line 을 fields[] 에 분리. 마지막 필드는 나머지 전체 (content-last).
 * 반환값: 파싱된 필드 수 */
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

/* DM_LIST_REQ|
 * → DM_LIST_RES|count:partner_id:nick:ts:unread:last_msg;...  (last_msg content-last)
 * messages.txt 에서 room_id=0 레코드를 스캔해 파트너별 최근 메시지 집계 */
static void handle_dm_list(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;

    typedef struct {
        char partner_id[21];
        char last_ts[20];
        char last_msg[256];
        int  last_msg_id;
        int  unread_count;
    } DmEntry;

    DmEntry entries[64];
    int     ecount = 0;

    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    FILE *fp = fopen(FILE_MESSAGES, "r");
    if (fp) {
        /* messages.txt 포맷:
         * id//room_id//from_id//to_id//reply_to//msg_type//is_deleted//created_at//edited_at//content */
        char line[MAX_PKT_SIZE + 256];
        while (fgets(line, sizeof(line), fp)) {
            if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
            char tmp[MAX_PKT_SIZE + 256];
            strncpy(tmp, line, sizeof(tmp) - 1);
            tmp[sizeof(tmp) - 1] = '\0';
            char *f[11];
            if (split_fields(tmp, f, 10) < 10) continue;
            if (atoi(f[1]) != 0) continue;       /* room_id != 0 → 그룹 메시지 */
            if (atoi(f[6]) != 0) continue;       /* is_deleted */

            const char *from_id = f[2];
            const char *to_id   = f[3];
            int msg_id = atoi(f[0]);

            const char *partner = NULL;
            if (strcmp(from_id, sess->user_id) == 0) partner = to_id;
            else if (strcmp(to_id, sess->user_id) == 0) partner = from_id;
            else continue;

            int i, found_idx = -1;
            for (i = 0; i < ecount; i++) {
                if (strcmp(entries[i].partner_id, partner) == 0) {
                    found_idx = i; break;
                }
            }
            if (found_idx < 0) {
                if (ecount >= 64) continue;
                found_idx = ecount++;
                strncpy(entries[found_idx].partner_id, partner, 20);
                entries[found_idx].partner_id[20] = '\0';
                entries[found_idx].last_msg_id = -1;
                entries[found_idx].unread_count = 0;
            }
            if (msg_id > entries[found_idx].last_msg_id) {
                entries[found_idx].last_msg_id = msg_id;
                strncpy(entries[found_idx].last_ts, f[7], 19);
                entries[found_idx].last_ts[19] = '\0';
                strncpy(entries[found_idx].last_msg, f[9], 255);
                entries[found_idx].last_msg[255] = '\0';
            }
            /* 미읽음 카운트: to_id 가 나이고 g_dm_reads 에 없는 메시지 */
            if (strcmp(to_id, sess->user_id) == 0) {
                int already_read = 0;
                int k;
                for (k = 0; k < g_dm_read_count; k++) {
                    if (g_dm_reads[k].msg_id == msg_id &&
                        strcmp(g_dm_reads[k].reader_id, sess->user_id) == 0) {
                        already_read = 1;
                        break;
                    }
                }
                if (!already_read) entries[found_idx].unread_count++;
            }
        }
        fclose(fp);
    }
    ReleaseMutex(g_file_mutex);

    char buf[MAX_PKT_SIZE];
    int  off = snprintf(buf, sizeof(buf) - 2, DM_LIST_RES "|%d", ecount);
    int  i;
    for (i = 0; i < ecount && off < (int)sizeof(buf) - 128; i++) {
        char nick[21];
        get_nickname(entries[i].partner_id, nick);
        /* i==0: count 와 첫 항목 사이 ':', i>0: 항목 사이 ';' */
        buf[off++] = (i == 0) ? ':' : ';';
        off += snprintf(buf + off, sizeof(buf) - off,
                        "%s:%s:%s:%d:%s",
                        entries[i].partner_id, nick,
                        entries[i].last_ts, entries[i].unread_count,
                        entries[i].last_msg);
    }
    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->sock, buf, off, 0);
}

/* DM_HISTORY_REQ|partner_id:count
 * → DM_HISTORY_RES|count:msg_id:from_id:ts:read:content;...  (content-last)
 * messages.txt 에서 해당 파트너와의 DM 이력 반환 */
static void handle_dm_history(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *with_id = strtok(payload, ":");
    char *count_s = strtok(NULL, "");
    if (!with_id) {
        send_packet(sess->sock, DM_HISTORY_RES "|0");
        return;
    }
    int n = (int)strlen(with_id);
    while (n > 0 && (with_id[n-1] == '\r' || with_id[n-1] == '\n'))
        with_id[--n] = '\0';

    int limit = count_s ? atoi(count_s) : 50;
    if (limit <= 0 || limit > 100) limit = 50;

    typedef struct {
        int  id;
        char from_id[21];
        char ts[20];
        char content[512];
        int  directed_to_me;   /* to_id == sess->user_id 이면 1 */
        int  read;             /* 내가 이미 읽은 메시지이면 1 */
    } HistEntry;

    HistEntry hist[100];
    int hcount = 0;

    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    FILE *fp = fopen(FILE_MESSAGES, "r");
    if (fp) {
        char line[MAX_PKT_SIZE + 256];
        while (fgets(line, sizeof(line), fp) && hcount < 100) {
            if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
            char tmp[MAX_PKT_SIZE + 256];
            strncpy(tmp, line, sizeof(tmp) - 1);
            tmp[sizeof(tmp) - 1] = '\0';
            char *f[11];
            if (split_fields(tmp, f, 10) < 10) continue;
            if (atoi(f[1]) != 0) continue;       /* room_id != 0 */
            if (atoi(f[6]) != 0) continue;       /* is_deleted */

            const char *from = f[2], *to = f[3];
            int match = (strcmp(from, sess->user_id) == 0 && strcmp(to, with_id) == 0) ||
                        (strcmp(from, with_id) == 0 && strcmp(to, sess->user_id) == 0);
            if (!match) continue;

            hist[hcount].id = atoi(f[0]);
            strncpy(hist[hcount].from_id, from, 20);
            hist[hcount].from_id[20] = '\0';
            strncpy(hist[hcount].ts, f[7], 19);
            hist[hcount].ts[19] = '\0';
            strncpy(hist[hcount].content, f[9], 511);
            hist[hcount].content[511] = '\0';
            hist[hcount].directed_to_me = (strcmp(to, sess->user_id) == 0);
            /* 읽음 여부: 내가 보낸 메시지는 항상 1, 받은 메시지는 g_dm_reads 확인 */
            if (!hist[hcount].directed_to_me) {
                hist[hcount].read = 1;
            } else {
                int k;
                hist[hcount].read = 0;
                for (k = 0; k < g_dm_read_count; k++) {
                    if (g_dm_reads[k].msg_id == hist[hcount].id &&
                        strcmp(g_dm_reads[k].reader_id, sess->user_id) == 0) {
                        hist[hcount].read = 1;
                        break;
                    }
                }
            }
            hcount++;
        }
        fclose(fp);
    }
    ReleaseMutex(g_file_mutex);

    /* 파일은 시간순 오름차순 — 마지막 limit 개만 반환 */
    int start = (hcount > limit) ? hcount - limit : 0;
    int count = hcount - start;

    char buf[MAX_PKT_SIZE];
    int  off = snprintf(buf, sizeof(buf) - 2, DM_HISTORY_RES "|%d", count);
    int  i;
    for (i = start; i < hcount && off < (int)sizeof(buf) - 128; i++) {
        /* i==start: count 와 첫 항목 사이 ':', 그 이후: 항목 사이 ';' */
        buf[off++] = (i == start) ? ':' : ';';
        off += snprintf(buf + off, sizeof(buf) - off,
                        "%d:%s:%s:%d:%s",
                        hist[i].id, hist[i].from_id,
                        hist[i].ts, hist[i].read, hist[i].content);
    }
    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->sock, buf, off, 0);

    /* 나에게 온 메시지를 읽음 처리하고 파트너에게 DM_READ_NOTIFY 전송 */
    {
        char read_ts[20];
        int  max_read_id = 0;
        int  k;
        get_current_timestamp(read_ts);

        /* MUTEX: g_file_mutex */
        WaitForSingleObject(g_file_mutex, INFINITE);
        for (i = start; i < hcount; i++) {
            if (!hist[i].directed_to_me) continue;
            int already = 0;
            for (k = 0; k < g_dm_read_count; k++) {
                if (g_dm_reads[k].msg_id == hist[i].id &&
                    strcmp(g_dm_reads[k].reader_id, sess->user_id) == 0) {
                    already = 1; break;
                }
            }
            if (!already && g_dm_read_count < MAX_DM_READS) {
                DmReadRecord *r = &g_dm_reads[g_dm_read_count];
                memset(r, 0, sizeof(*r));
                r->msg_id = hist[i].id;
                strncpy(r->reader_id, sess->user_id, 20);
                strncpy(r->read_at, read_ts, 19);
                g_dm_read_count++;
                append_dm_read(FILE_DM_READS, r);
            }
            if (hist[i].id > max_read_id) max_read_id = hist[i].id;
        }
        ReleaseMutex(g_file_mutex);

        if (max_read_id > 0) {
            char nbuf[128];
            int  nlen = snprintf(nbuf, sizeof(nbuf) - 2,
                                 DM_READ_NOTIFY "|%s:%d",
                                 sess->user_id, max_read_id);
            nbuf[nlen++] = '\n'; nbuf[nlen] = '\0';
            send_to_user(with_id, nbuf);
        }
    }
}

void dm_init(void) {
    register_handler(DM_SEND,        handle_dm_send);
    register_handler(DM_LIST_REQ,    handle_dm_list);
    register_handler(DM_HISTORY_REQ, handle_dm_history);
}
