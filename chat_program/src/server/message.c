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
#include "user_store.h"
#include "message.h"

/* 긴 시간 문자열에서 HH:MM 부분만 뽑는다. */
static void msg_ts_hhmm(const char *ts_long, char out[6]) {
    int h = 0, m = 0;
    sscanf(ts_long, "%*d-%*d-%*d %d:%d", &h, &m);
    snprintf(out, 6, "%02d.%02d", h, m);
}

/* "//" 구분자로 line 을 최대 max_f 개 필드로 분리. 마지막 필드는 content-last.
 * 반환값: 파싱된 필드 수. line 은 파괴적으로 수정된다. */
/* 저장된 한 줄을 파일 구분자로 나누어 배열에 담는다. */
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
/* 저장된 시간 문자열을 비교 가능한 시간 값으로 바꾼다. */
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
/* 본인이 쓴 메시지 수정 요청을 처리한다. */
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
/* 본인이 쓴 메시지 삭제 요청을 처리한다. */
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

/* MSG_REPLY|room_id:reply_to_id:content  (content-last)
 * 지정 메시지를 인용한 일반 메시지 전송. reply_to_id를 reply_to 필드에 저장.
 * → ROOM_MSG_RECV|room_id:nick:ts:msg_id:reply_to:msg_type:content */
/* 특정 메시지에 답장하는 요청을 처리한다. */
static void handle_msg_reply(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s  = strtok(payload, ":");
    char *reply_to_s = strtok(NULL,    ":");
    char *content    = strtok(NULL,    "");
    if (!room_id_s || !reply_to_s || !content) return;

    int n = (int)strlen(content);
    while (n > 0 && (content[n-1] == '\r' || content[n-1] == '\n'))
        content[--n] = '\0';
    if (n == 0) return;

    int room_id  = atoi(room_id_s);
    int reply_to = atoi(reply_to_s);

    /* MUTEX: g_sessions_mutex — 멤버십 확인 + is_open 캡처 */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int is_member = 0, is_open_room = 0;
    {
        int idx = find_room_idx(room_id);
        if (idx >= 0) {
            int j;
            is_open_room = g_rooms[idx].info.is_open;
            for (j = 0; j < g_rooms[idx].member_count; j++) {
                if (strcmp(g_rooms[idx].member_ids[j], sess->user_id) == 0) {
                    is_member = 1; break;
                }
            }
        }
    }
    ReleaseMutex(g_sessions_mutex);
    if (!is_member) return;

    char nick[21], ts_long[20], ts_short[6];
    get_nickname(sess->user_id, nick);
    /* 오픈채팅 방이면 open_nick 우선 조회 */
    if (is_open_room) {
        int mi;
        for (mi = 0; mi < g_room_member_count; mi++) {
            if (g_room_members[mi].room_id == room_id &&
                strcmp(g_room_members[mi].user_id, sess->user_id) == 0 &&
                g_room_members[mi].open_nick[0] != '\0') {
                strncpy(nick, g_room_members[mi].open_nick, 20);
                nick[20] = '\0';
                break;
            }
        }
    }
    get_current_timestamp(ts_long);
    msg_ts_hhmm(ts_long, ts_short);

    int msg_id;
    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    msg_id = g_next_msg_id++;
    {
        MessageRecord m;
        memset(&m, 0, sizeof(m));
        m.id          = msg_id;
        m.room_id     = room_id;
        m.reply_to_id = reply_to;
        strncpy(m.from_id,    sess->user_id, 20);
        strncpy(m.from_nick,  nick, 20);
        m.msg_type = MSG_TYPE_NORMAL;
        strncpy(m.created_at, ts_long, 19);
        strncpy(m.content,    content, MAX_PKT_SIZE - 1);
        append_message(FILE_MESSAGES, &m);
    }
    ReleaseMutex(g_file_mutex);

    char buf[MAX_PKT_SIZE];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         ROOM_MSG_RECV "|%d:%s:%s:%d:%d:%d:%s",
                         room_id, nick, ts_short, msg_id, reply_to,
                         MSG_TYPE_NORMAL, content);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        broadcast_to_room(room_id, buf);
    }
}

/* MSG_SEARCH|room_id:keyword  (keyword content-last)
 * g_messages[]에서 room_id+keyword 일치 메시지 검색.
 * → MSG_SEARCH_RES|count:msg_id:from_nick:ts:content;...  (count-first, content-last) */
/* 방 안 메시지에서 검색어가 들어간 내용을 찾는다. */
static void handle_msg_search(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *keyword   = strtok(NULL,    "");
    if (!room_id_s || !keyword) {
        send_packet(sess->sock, MSG_SEARCH_RES "|0");
        return;
    }

    int n = (int)strlen(keyword);
    while (n > 0 && (keyword[n-1] == '\r' || keyword[n-1] == '\n'))
        keyword[--n] = '\0';
    if (n == 0) {
        send_packet(sess->sock, MSG_SEARCH_RES "|0");
        return;
    }

    int room_id = atoi(room_id_s);
    int i, cnt = 0;

    /* 1패스: 결과 수 카운트 */
    for (i = 0; i < g_msg_count; i++) {
        MessageRecord *m = &g_messages[i];
        if (m->room_id != room_id) continue;
        if (m->is_deleted) continue;
        if (m->msg_type == MSG_TYPE_SYSTEM) continue;
        if (strstr(m->content, keyword)) cnt++;
    }

    /* 2패스: 패킷 빌드  MSG_SEARCH_RES|count:entry1;entry2;... */
    char buf[MAX_PKT_SIZE];
    int  off = snprintf(buf, sizeof(buf) - 2, MSG_SEARCH_RES "|%d", cnt);
    int  found = 0;

    for (i = 0; i < g_msg_count && off < (int)sizeof(buf) - 200; i++) {
        MessageRecord *m = &g_messages[i];
        if (m->room_id != room_id) continue;
        if (m->is_deleted) continue;
        if (m->msg_type == MSG_TYPE_SYSTEM) continue;
        if (!strstr(m->content, keyword)) continue;

        char ts_s[6];
        msg_ts_hhmm(m->created_at, ts_s);
        const char *nick = m->from_nick[0] ? m->from_nick : m->from_id;

        if (found == 0)
            off += snprintf(buf + off, sizeof(buf) - off - 2,
                            ":%d:%s:%s:%s", m->id, nick, ts_s, m->content);
        else
            off += snprintf(buf + off, sizeof(buf) - off - 2,
                            ";%d:%s:%s:%s", m->id, nick, ts_s, m->content);
        found++;
    }

    if (off > 0 && off < (int)sizeof(buf) - 2) {
        buf[off++] = '\n'; buf[off] = '\0';
        send(sess->sock, buf, off, 0);
    }
}

/* WHISPER|room_id:target_id:content  (content content-last)
 * 같은 방에 있는 target_id 에게만 귓속말 전송.
 * → 송신자와 수신자 모두에게 WHISPER_RECV|room_id:from_id:from_nick:content */
/* 같은 방 사용자에게 보내는 귓속말을 처리한다. */
static void handle_whisper(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *target_id = strtok(NULL,    ":");
    char *content   = strtok(NULL,    "");
    if (!room_id_s || !target_id || !content) return;

    int n = (int)strlen(content);
    while (n > 0 && (content[n-1] == '\r' || content[n-1] == '\n'))
        content[--n] = '\0';
    if (n == 0) return;

    n = (int)strlen(target_id);
    while (n > 0 && (target_id[n-1] == '\r' || target_id[n-1] == '\n'))
        target_id[--n] = '\0';

    int room_id = atoi(room_id_s);

    /* 송신자가 해당 방의 멤버인지, 수신자도 같은 방에 있는지 확인 */
    int sender_in_room = 0, target_in_room = 0;
    int i;
    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!g_sessions[i].active) continue;
        if (strcmp(g_sessions[i].user_id, sess->user_id) == 0 &&
            g_sessions[i].room_id == room_id)
            sender_in_room = 1;
        if (strcmp(g_sessions[i].user_id, target_id) == 0 &&
            g_sessions[i].room_id == room_id)
            target_in_room = 1;
    }
    ReleaseMutex(g_sessions_mutex);

    if (!sender_in_room || !target_in_room) return;

    char nick[21];
    get_nickname(sess->user_id, nick);

    char buf[MAX_PKT_SIZE];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         WHISPER_RECV "|%d:%s:%s:%s",
                         room_id, sess->user_id, nick, content);
    if (mlen <= 0 || mlen >= (int)sizeof(buf) - 2) return;
    buf[mlen++] = '\n'; buf[mlen] = '\0';

    /* 수신자와 송신자 모두에게 전달 */
    send_to_user(target_id, buf);
    if (strcmp(sess->user_id, target_id) != 0)
        send_to_user(sess->user_id, buf);
}

/* MSG_PIN|room_id:msg_id
 * 방장 또는 관리자만 가능. 핀 메시지 갱신 후 MSG_PIN_NOTIFY|room_id:msg_id 브로드캐스트.
 * msg_id=0 이면 핀 해제. */
/* 방의 고정 메시지를 바꾸고 멤버들에게 알린다. */
static void handle_msg_pin(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *msg_id_s  = strtok(NULL, "");
    if (!room_id_s || !msg_id_s) return;

    int n = (int)strlen(msg_id_s);
    while (n > 0 && (msg_id_s[n-1] == '\r' || msg_id_s[n-1] == '\n'))
        msg_id_s[--n] = '\0';

    int room_id = atoi(room_id_s);
    int msg_id  = atoi(msg_id_s);

    /* MUTEX: g_sessions_mutex — 권한 확인 및 핀 메시지 갱신 */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0) { ReleaseMutex(g_sessions_mutex); return; }

    /* 방장 또는 관리자 확인 */
    int is_owner = (strcmp(g_rooms[idx].info.owner_id, sess->user_id) == 0);
    int is_admin = 0;
    int j;
    for (j = 0; j < g_rooms[idx].member_count; j++) {
        if (strcmp(g_rooms[idx].member_ids[j], sess->user_id) == 0) {
            if (g_rooms[idx].admin_flags[j]) is_admin = 1;
            break;
        }
    }
    if (!is_owner && !is_admin) { ReleaseMutex(g_sessions_mutex); return; }

    g_rooms[idx].info.pinned_msg_id = msg_id;
    ReleaseMutex(g_sessions_mutex);

    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    save_rooms(FILE_ROOMS);
    ReleaseMutex(g_file_mutex);

    char buf[64];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         MSG_PIN_NOTIFY "|%d:%d", room_id, msg_id);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        broadcast_to_room(room_id, buf);
    }
}

/* 메시지 관련 패킷 처리 함수를 등록한다. */
void message_init(void) {
    register_handler(MSG_EDIT,   handle_msg_edit);
    register_handler(MSG_DELETE, handle_msg_delete);
    register_handler(MSG_REPLY,  handle_msg_reply);
    register_handler(MSG_SEARCH, handle_msg_search);
    register_handler(WHISPER,    handle_whisper);
    register_handler(MSG_PIN,    handle_msg_pin);
}
