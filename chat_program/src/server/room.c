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
#include "room.h"

/* HH.MM 포맷 타임스탬프 — ':' 대신 '.' 사용하여 패킷 필드 구분자 충돌 방지 */
static void get_time_hhmm(char out[6]) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(out, 6, "%02d.%02d", st.wHour, st.wMinute);
}

/* 방 멤버 여부 확인. g_sessions_mutex 를 내부 획득한다. */
int is_room_member(int room_id, const char *user_id) {
    int i, j, found = 0;
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < g_room_count; i++) {
        if (g_rooms[i].info.id == room_id && !g_rooms[i].info.is_deleted) {
            for (j = 0; j < g_rooms[i].member_count; j++) {
                if (strcmp(g_rooms[i].member_ids[j], user_id) == 0) {
                    found = 1; break;
                }
            }
            break;
        }
    }
    ReleaseMutex(g_sessions_mutex);
    return found;
}

/* ROOM_LIST_REQ|open | group | (없으면 전체) */
static void handle_room_list(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    int filter = -1;
    if (payload) {
        if (strncmp(payload, "open",  4) == 0) filter = 1;
        else if (strncmp(payload, "group", 5) == 0) filter = 0;
    }

    char buf[MAX_PKT_SIZE];
    int  off = 0, cnt = 0, i;

    off += snprintf(buf + off, sizeof(buf) - off, ROOM_LIST_RES "|");

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < g_room_count && off < (int)sizeof(buf) - 128; i++) {
        if (g_rooms[i].info.is_deleted) continue;
        if (filter == 1 && !g_rooms[i].info.is_open)  continue;
        if (filter == 0 &&  g_rooms[i].info.is_open)  continue;

        int has_pw = (g_rooms[i].info.pw_hash[0] != '\0');
        if (cnt > 0) buf[off++] = ';';
        off += snprintf(buf + off, sizeof(buf) - off,
                        "%d:%s:%d:%d:%d:%d:%s",
                        g_rooms[i].info.id,
                        g_rooms[i].info.name,
                        g_rooms[i].member_count,
                        g_rooms[i].info.max_members,
                        has_pw,
                        g_rooms[i].info.is_open,
                        g_rooms[i].info.topic);
        cnt++;
    }
    ReleaseMutex(g_sessions_mutex);

    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->sock, buf, off, 0);
}

/* ROOM_CREATE|name:max_users:is_open:pw_hash (content-last)
 * → ROOM_CREATE_RES|1:room_id  or  ROOM_CREATE_RES|0 */
static void handle_room_create(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *name      = strtok(payload, ":");
    char *max_s     = strtok(NULL,    ":");
    char *is_open_s = strtok(NULL,    ":");
    char *pw_hash   = strtok(NULL,    "");
    int   n;

    if (!name || strlen(name) == 0 || strlen(name) > 30) {
        send_packet(sess->sock, ROOM_CREATE_RES "|%d", ROOM_CREATE_FAIL);
        return;
    }
    if (has_forbidden_char(name)) {
        send_packet(sess->sock, ROOM_CREATE_RES "|%d", ROOM_CREATE_FAIL);
        return;
    }

    int max_users = max_s ? atoi(max_s) : 10;
    if (max_users < 2 || max_users > MAX_ROOM_MEMBERS) max_users = 10;
    int is_open = is_open_s ? atoi(is_open_s) : 0;

    if (pw_hash) {
        n = (int)strlen(pw_hash);
        while (n > 0 && (pw_hash[n-1] == '\r' || pw_hash[n-1] == '\n'))
            pw_hash[--n] = '\0';
    }

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    if (g_room_count >= MAX_ROOMS) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, ROOM_CREATE_RES "|%d", ROOM_CREATE_FAIL);
        return;
    }

    RoomInfo *ri = &g_rooms[g_room_count];
    memset(ri, 0, sizeof(*ri));
    ri->info.id          = g_next_room_id++;
    strncpy(ri->info.name,     name, 30);
    strncpy(ri->info.owner_id, sess->user_id, 20);
    ri->info.max_members = max_users;
    ri->info.is_open     = is_open;
    if (pw_hash && pw_hash[0] != '\0')
        strncpy(ri->info.pw_hash, pw_hash, 64);
    get_current_timestamp(ri->info.created_at);

    strncpy(ri->member_ids[0], sess->user_id, 20);
    ri->member_count = 1;

    int room_id  = ri->info.id;
    int room_idx = g_room_count++;
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    append_room(FILE_ROOMS, &g_rooms[room_idx].info);
    {
        RoomMemberRecord mem;
        memset(&mem, 0, sizeof(mem));
        mem.room_id  = room_id;
        strncpy(mem.user_id, sess->user_id, 20);
        mem.is_admin = 1;
        get_current_timestamp(mem.joined_at);
        append_room_member(FILE_ROOM_MEMBERS, &mem);
    }
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, ROOM_CREATE_RES "|%d:%d", ROOM_CREATE_OK, room_id);
}

/* ROOM_JOIN|room_id  or  ROOM_JOIN|room_id:pw_hash
 * → ROOM_JOIN_RES|0:room_id:name (content-last)  or  ROOM_JOIN_RES|error_code */
static void handle_room_join(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *pw_hash   = strtok(NULL, "");
    if (!room_id_s) {
        send_packet(sess->sock, ROOM_JOIN_RES "|%d", ROOM_JOIN_NOT_FOUND);
        return;
    }

    int n;
    if (pw_hash) {
        n = (int)strlen(pw_hash);
        while (n > 0 && (pw_hash[n-1] == '\r' || pw_hash[n-1] == '\n'))
            pw_hash[--n] = '\0';
    }

    int room_id = atoi(room_id_s);

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, ROOM_JOIN_RES "|%d", ROOM_JOIN_NOT_FOUND);
        return;
    }

    RoomInfo *ri = &g_rooms[idx];

    if (ri->info.pw_hash[0] != '\0') {
        if (!pw_hash || strcmp(ri->info.pw_hash, pw_hash) != 0) {
            ReleaseMutex(g_sessions_mutex);
            send_packet(sess->sock, ROOM_JOIN_RES "|%d", ROOM_JOIN_WRONG_PW);
            return;
        }
    }

    int j, already = 0;
    for (j = 0; j < ri->member_count; j++) {
        if (strcmp(ri->member_ids[j], sess->user_id) == 0) { already = 1; break; }
    }

    if (!already && ri->member_count >= ri->info.max_members) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, ROOM_JOIN_RES "|%d", ROOM_JOIN_FULL);
        return;
    }

    if (!already && ri->member_count < MAX_ROOM_MEMBERS) {
        strncpy(ri->member_ids[ri->member_count], sess->user_id, 20);
        ri->member_count++;
    }

    char room_name[31];
    strncpy(room_name, ri->info.name, 30);
    room_name[30] = '\0';

    sess->room_id = room_id;
    ReleaseMutex(g_sessions_mutex);

    if (!already) {
        WaitForSingleObject(g_file_mutex, INFINITE);
        {
            RoomMemberRecord mem;
            memset(&mem, 0, sizeof(mem));
            mem.room_id = room_id;
            strncpy(mem.user_id, sess->user_id, 20);
            get_current_timestamp(mem.joined_at);
            append_room_member(FILE_ROOM_MEMBERS, &mem);
        }
        ReleaseMutex(g_file_mutex);

        char nick[21], ts[6];
        get_nickname(sess->user_id, nick);
        get_time_hhmm(ts);

        char buf[512];
        int  mlen = snprintf(buf, sizeof(buf) - 2,
                             ROOM_MSG_RECV "|%d:[시스템]:%s:0:0:%d:%s님이 입장했습니다.",
                             room_id, ts, MSG_TYPE_SYSTEM, nick);
        if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
            buf[mlen++] = '\n'; buf[mlen] = '\0';
            broadcast_to_room(room_id, buf);
        }
    }

    send_packet(sess->sock, ROOM_JOIN_RES "|%d:%d:%s",
                ROOM_JOIN_OK, room_id, room_name);
}

/* ROOM_LEAVE| — 멤버 제거, 퇴장 메시지 브로드캐스트 */
static void handle_room_leave(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;

    int room_id = sess->room_id;
    if (room_id == 0) return;

    char nick[21];
    get_nickname(sess->user_id, nick);

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx >= 0) {
        RoomInfo *ri = &g_rooms[idx];
        int j, k;
        for (j = 0; j < ri->member_count; j++) {
            if (strcmp(ri->member_ids[j], sess->user_id) == 0) {
                for (k = j; k < ri->member_count - 1; k++)
                    memcpy(ri->member_ids[k], ri->member_ids[k+1], 21);
                ri->member_count--;
                break;
            }
        }
    }
    sess->room_id = 0;  /* 클리어 후 broadcast → 본인은 미수신 */
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_room_members(FILE_ROOM_MEMBERS);
    ReleaseMutex(g_file_mutex);

    char ts[6];
    get_time_hhmm(ts);
    char buf[512];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         ROOM_MSG_RECV "|%d:[시스템]:%s:0:0:%d:%s님이 퇴장했습니다.",
                         room_id, ts, MSG_TYPE_SYSTEM, nick);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        broadcast_to_room(room_id, buf);
    }
}

/* 메시지 내 @user_id 멘션을 감지해 해당 유저에게 NOTIFY 전송.
 * g_sessions_mutex / g_file_mutex 를 보유하지 않은 상태에서 호출한다. */
static void detect_mention(int room_id, const char *content,
                            const char *sender_nick) {
    const char *p = content;
    while ((p = strchr(p, '@')) != NULL) {
        p++;
        int mlen = 0;
        while (p[mlen] != '\0' && p[mlen] != ' ' && p[mlen] != '\n'
               && p[mlen] != '\r' && mlen < 20)
            mlen++;
        if (mlen == 0) continue;

        char mentioned[21];
        strncpy(mentioned, p, mlen);
        mentioned[mlen] = '\0';

        if (!find_user_by_id(mentioned)) { p += mlen; continue; }
        if (!is_room_member(room_id, mentioned)) { p += mlen; continue; }

        char buf[MAX_PKT_SIZE];
        int blen = snprintf(buf, sizeof(buf) - 2,
                            NOTIFY "|MENTION:%s님이 나를 멘션했습니다: %s",
                            sender_nick, content);
        if (blen > 0 && blen < (int)sizeof(buf) - 2) {
            buf[blen++] = '\n'; buf[blen] = '\0';
            send_to_user(mentioned, buf);
        }
        p += mlen;
    }
}

/* ROOM_MSG|room_id:content (content-last)
 * → ROOM_MSG_RECV|room_id:nick:HH.MM:msg_id:0:msg_type:content 브로드캐스트 */
static void handle_room_msg(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *content   = strtok(NULL, "");
    if (!room_id_s || !content) return;

    int n = (int)strlen(content);
    while (n > 0 && (content[n-1] == '\r' || content[n-1] == '\n'))
        content[--n] = '\0';
    if (n == 0) return;

    int room_id = atoi(room_id_s);

    /* /me 접두사 감지 → MSG_TYPE_ME로 전환 */
    int         msg_type       = MSG_TYPE_NORMAL;
    const char *actual_content = content;
    if (strncmp(content, "/me ", 4) == 0 && content[4] != '\0') {
        msg_type       = MSG_TYPE_ME;
        actual_content = content + 4;
    }

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
    /* 오픈채팅 방이면 open_nick 우선 조회 (g_room_members 는 읽기 전용 접근) */
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
    get_time_hhmm(ts_short);

    /* 이모티콘 코드 변환 (:smile: → (^_^) 등) */
    char emo_content[MAX_PKT_SIZE];
    convert_emoticons(actual_content, emo_content, sizeof(emo_content));

    int msg_id;
    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    msg_id = g_next_msg_id++;
    {
        MessageRecord m;
        memset(&m, 0, sizeof(m));
        m.id      = msg_id;
        m.room_id = room_id;
        strncpy(m.from_id,   sess->user_id, 20);
        strncpy(m.from_nick, nick, 20);
        m.msg_type = msg_type;
        strncpy(m.created_at, ts_long, 19);
        strncpy(m.content, emo_content, MAX_PKT_SIZE - 1);
        append_message(FILE_MESSAGES, &m);
    }
    ReleaseMutex(g_file_mutex);

    char buf[MAX_PKT_SIZE];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         ROOM_MSG_RECV "|%d:%s:%s:%d:0:%d:%s",
                         room_id, nick, ts_short, msg_id, msg_type, emo_content);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        broadcast_to_room(room_id, buf);
    }

    /* @멘션 감지 — 일반 메시지에 한해 실행 */
    if (msg_type == MSG_TYPE_NORMAL)
        detect_mention(room_id, emo_content, nick);
}

/* "//" 구분자로 line 을 최대 max_f 개 필드로 분리 (content-last). */
static int room_split_fields(char *line, char **f, int max_f) {
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

/* "YYYY-MM-DD HH:MM:SS" → "HH.MM" (패킷 ':' 충돌 방지) */
static void ts_to_hhmm(const char *ts, char out[6]) {
    int h = 0, m = 0;
    sscanf(ts, "%*d-%*d-%*d %d:%d", &h, &m);
    snprintf(out, 6, "%02d.%02d", h, m);
}

/* ROOM_HISTORY_REQ|room_id:<count>
 * → ROOM_HISTORY_RES|count:msg_id:from_nick:timestamp:reply_to_id:msg_type:content;... */
static void handle_room_history(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *rid_s   = strtok(payload, ":");
    char *count_s = strtok(NULL, "");
    if (!rid_s) { send_packet(sess->sock, ROOM_HISTORY_RES "|0"); return; }

    int room_id = atoi(rid_s);
    int limit   = count_s ? atoi(count_s) : 50;
    if (limit <= 0 || limit > 100) limit = 50;

    /* MUTEX: g_sessions_mutex — 멤버십 확인 + is_open 캡처 */
    int is_member = 0, is_open_room = 0;
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    {
        int idx = find_room_idx(room_id);
        if (idx >= 0 && !g_rooms[idx].info.is_deleted) {
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
    if (!is_member) {
        send_packet(sess->sock, ROOM_HISTORY_RES "|0");
        return;
    }

    typedef struct {
        int  id;
        char from_nick[21];
        char ts[6];
        char reply_to[12];
        int  msg_type;
        char content[MAX_PKT_SIZE];
    } HistEntry;

    HistEntry *hist = (HistEntry *)malloc(sizeof(HistEntry) * 100);
    if (!hist) { send_packet(sess->sock, ROOM_HISTORY_RES "|0"); return; }
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
            if (room_split_fields(tmp, f, 10) < 10) continue;
            if (atoi(f[1]) != room_id) continue;
            if (atoi(f[6]) != 0) continue;   /* is_deleted */

            hist[hcount].id = atoi(f[0]);
            char nick[21];
            get_nickname(f[2], nick);
            /* 오픈채팅 방이면 open_nick 우선 조회 */
            if (is_open_room) {
                int mi;
                for (mi = 0; mi < g_room_member_count; mi++) {
                    if (g_room_members[mi].room_id == room_id &&
                        strcmp(g_room_members[mi].user_id, f[2]) == 0 &&
                        g_room_members[mi].open_nick[0] != '\0') {
                        strncpy(nick, g_room_members[mi].open_nick, 20);
                        nick[20] = '\0';
                        break;
                    }
                }
            }
            strncpy(hist[hcount].from_nick, nick, 20);
            hist[hcount].from_nick[20] = '\0';
            ts_to_hhmm(f[7], hist[hcount].ts);
            strncpy(hist[hcount].reply_to, f[4], 11);
            hist[hcount].reply_to[11] = '\0';
            hist[hcount].msg_type = atoi(f[5]);
            strncpy(hist[hcount].content, f[9], MAX_PKT_SIZE - 1);
            hist[hcount].content[MAX_PKT_SIZE - 1] = '\0';
            hcount++;
        }
        fclose(fp);
    }
    ReleaseMutex(g_file_mutex);

    int start = (hcount > limit) ? hcount - limit : 0;
    int count = hcount - start;

    char buf[MAX_PKT_SIZE];
    int  off = snprintf(buf, sizeof(buf) - 2, ROOM_HISTORY_RES "|%d", count);
    int  i;
    for (i = start; i < hcount && off < (int)sizeof(buf) - 128; i++) {
        buf[off++] = (i == start) ? ':' : ';';
        off += snprintf(buf + off, sizeof(buf) - off,
                        "%d:%s:%s:%s:%d:%s",
                        hist[i].id, hist[i].from_nick,
                        hist[i].ts, hist[i].reply_to,
                        hist[i].msg_type, hist[i].content);
    }
    buf[off++] = '\n';
    buf[off]   = '\0';
    free(hist);
    send(sess->sock, buf, off, 0);
}

/* ROOM_INVITE|room_id:target_id
 * → ROOM_INVITE_RES|code (0=SENT, 1=NOT_FOUND, 2=ALREADY_MEMBER, 3=FULL)
 * 성공 시 대상 유저에게 ROOM_INVITE_NOTIFY|room_id:room_name:inviter_nick 전송 */
static void handle_room_invite(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *target_id = strtok(NULL, "");
    if (!room_id_s || !target_id) {
        send_packet(sess->sock, ROOM_INVITE_RES "|1");
        return;
    }

    int n = (int)strlen(target_id);
    while (n > 0 && (target_id[n-1] == '\r' || target_id[n-1] == '\n'))
        target_id[--n] = '\0';
    if (n == 0) {
        send_packet(sess->sock, ROOM_INVITE_RES "|1");
        return;
    }

    int room_id = atoi(room_id_s);

    if (!find_user_by_id(target_id)) {
        send_packet(sess->sock, ROOM_INVITE_RES "|1");  /* NOT_FOUND */
        return;
    }

    char room_name[31] = {0};

    /* MUTEX: g_sessions_mutex — 방 존재 및 멤버십 확인 */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0 || g_rooms[idx].info.is_deleted) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, ROOM_INVITE_RES "|1");
        return;
    }

    int j, sender_is_member = 0, target_is_member = 0;
    for (j = 0; j < g_rooms[idx].member_count; j++) {
        if (strcmp(g_rooms[idx].member_ids[j], sess->user_id) == 0) sender_is_member = 1;
        if (strcmp(g_rooms[idx].member_ids[j], target_id)     == 0) target_is_member = 1;
    }

    if (!sender_is_member) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, ROOM_INVITE_RES "|1");
        return;
    }
    if (target_is_member) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, ROOM_INVITE_RES "|2");  /* ALREADY_MEMBER */
        return;
    }
    if (g_rooms[idx].member_count >= g_rooms[idx].info.max_members) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, ROOM_INVITE_RES "|3");  /* FULL */
        return;
    }

    strncpy(room_name, g_rooms[idx].info.name, 30);
    room_name[30] = '\0';
    ReleaseMutex(g_sessions_mutex);

    /* MUTEX: g_file_mutex — 초대 레코드 기록 */
    WaitForSingleObject(g_file_mutex, INFINITE);
    {
        char ts[20];
        get_current_timestamp(ts);
        int inv_id = g_next_invite_id++;
        /* room_invites.txt 포맷: id//room_id//inviter_id//invitee_id//status//created_at */
        FILE *fp = fopen(FILE_ROOM_INVITES, "a");
        if (fp) {
            fprintf(fp, "%d//%d//%s//%s//0//%s\n",
                    inv_id, room_id, sess->user_id, target_id, ts);
            fclose(fp);
        }
    }
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, ROOM_INVITE_RES "|0");  /* SENT */

    /* 대상이 온라인이면 ROOM_INVITE_NOTIFY 전송 */
    char nick[21];
    get_nickname(sess->user_id, nick);
    char buf[256];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         ROOM_INVITE_NOTIFY "|%d:%s:%s",
                         room_id, room_name, nick);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        send_to_user(target_id, buf);
    }
}

/* ROOM_KICK|room_id:target_id  (방장 전용)
 * → 대상에게 ROOM_KICKED_NOTIFY|room_id:by_nick 전송 후 멤버 제거 */
static void handle_room_kick(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *target_id = strtok(NULL, "");
    if (!room_id_s || !target_id) return;

    int n = (int)strlen(target_id);
    while (n > 0 && (target_id[n-1] == '\r' || target_id[n-1] == '\n'))
        target_id[--n] = '\0';
    if (n == 0) return;

    int room_id = atoi(room_id_s);

    /* MUTEX: g_sessions_mutex — 권한 확인 및 멤버 제거 */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0 || g_rooms[idx].info.is_deleted) {
        ReleaseMutex(g_sessions_mutex);
        return;
    }

    /* 방장만 강퇴 가능 */
    if (strcmp(g_rooms[idx].info.owner_id, sess->user_id) != 0) {
        ReleaseMutex(g_sessions_mutex);
        return;
    }

    /* 자기 자신은 강퇴 불가 */
    if (strcmp(target_id, sess->user_id) == 0) {
        ReleaseMutex(g_sessions_mutex);
        return;
    }

    int j, k, found = 0;
    for (j = 0; j < g_rooms[idx].member_count; j++) {
        if (strcmp(g_rooms[idx].member_ids[j], target_id) == 0) {
            for (k = j; k < g_rooms[idx].member_count - 1; k++)
                memcpy(g_rooms[idx].member_ids[k], g_rooms[idx].member_ids[k+1], 21);
            g_rooms[idx].member_count--;
            found = 1;
            break;
        }
    }

    /* 강퇴 대상의 세션 room_id 초기화 */
    if (found) {
        int s;
        for (s = 0; s < MAX_CLIENTS; s++) {
            if (g_sessions[s].active &&
                strcmp(g_sessions[s].user_id, target_id) == 0) {
                g_sessions[s].room_id = 0;
                break;
            }
        }
    }
    ReleaseMutex(g_sessions_mutex);

    if (!found) return;

    /* MUTEX: g_file_mutex — 멤버 목록 저장 */
    WaitForSingleObject(g_file_mutex, INFINITE);
    save_room_members(FILE_ROOM_MEMBERS);
    ReleaseMutex(g_file_mutex);

    /* 강퇴된 사용자에게 알림 전송 */
    char nick[21];
    get_nickname(sess->user_id, nick);
    char buf[256];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         ROOM_KICKED_NOTIFY "|%d:%s",
                         room_id, nick);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        send_to_user(target_id, buf);
    }
}

/* ROOM_SET_NOTICE|room_id:notice  (notice content-last)
 * 방장 또는 관리자만 가능. 방 전체에 ROOM_NOTICE|room_id:notice 브로드캐스트. */
static void handle_room_set_notice(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *notice    = strtok(NULL, "");
    if (!room_id_s) return;

    int n;
    if (notice) {
        n = (int)strlen(notice);
        while (n > 0 && (notice[n-1] == '\r' || notice[n-1] == '\n'))
            notice[--n] = '\0';
    } else {
        notice = "";
    }

    int room_id = atoi(room_id_s);

    /* MUTEX: g_sessions_mutex */
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

    strncpy(g_rooms[idx].info.notice, notice, 255);
    g_rooms[idx].info.notice[255] = '\0';
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_rooms(FILE_ROOMS);
    ReleaseMutex(g_file_mutex);

    char buf[MAX_PKT_SIZE];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         ROOM_NOTICE "|%d:%s", room_id, notice);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        broadcast_to_room(room_id, buf);
    }
}

/* ROOM_GRANT_ADMIN|room_id:target_id  — 방장 전용
 * target_id 를 관리자로 승격한다. g_rooms[].admin_flags 및 g_room_members 갱신. */
static void handle_room_grant_admin(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *target_id = strtok(NULL, "");
    if (!room_id_s || !target_id) return;

    int n = (int)strlen(target_id);
    while (n > 0 && (target_id[n-1] == '\r' || target_id[n-1] == '\n'))
        target_id[--n] = '\0';
    if (n == 0) return;

    int room_id = atoi(room_id_s);
    int i;

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0 || strcmp(g_rooms[idx].info.owner_id, sess->user_id) != 0) {
        ReleaseMutex(g_sessions_mutex);
        return;
    }

    int j, found = 0;
    for (j = 0; j < g_rooms[idx].member_count; j++) {
        if (strcmp(g_rooms[idx].member_ids[j], target_id) == 0) {
            g_rooms[idx].admin_flags[j] = 1;
            found = 1;
            break;
        }
    }
    ReleaseMutex(g_sessions_mutex);
    if (!found) return;

    /* g_room_members 동기화 */
    for (i = 0; i < g_room_member_count; i++) {
        if (g_room_members[i].room_id == room_id &&
            strcmp(g_room_members[i].user_id, target_id) == 0) {
            g_room_members[i].is_admin = 1;
            break;
        }
    }

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_room_members(FILE_ROOM_MEMBERS);
    ReleaseMutex(g_file_mutex);
}

/* ROOM_REVOKE_ADMIN|room_id:target_id  — 방장 전용
 * target_id 의 관리자 권한을 박탈한다. 방장 자신은 박탈 불가. */
static void handle_room_revoke_admin(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *target_id = strtok(NULL, "");
    if (!room_id_s || !target_id) return;

    int n = (int)strlen(target_id);
    while (n > 0 && (target_id[n-1] == '\r' || target_id[n-1] == '\n'))
        target_id[--n] = '\0';
    if (n == 0) return;

    int room_id = atoi(room_id_s);
    int i;

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0 || strcmp(g_rooms[idx].info.owner_id, sess->user_id) != 0) {
        ReleaseMutex(g_sessions_mutex);
        return;
    }
    /* 방장 자신의 권한은 박탈 불가 */
    if (strcmp(target_id, sess->user_id) == 0) {
        ReleaseMutex(g_sessions_mutex);
        return;
    }

    int j, found = 0;
    for (j = 0; j < g_rooms[idx].member_count; j++) {
        if (strcmp(g_rooms[idx].member_ids[j], target_id) == 0) {
            g_rooms[idx].admin_flags[j] = 0;
            found = 1;
            break;
        }
    }
    ReleaseMutex(g_sessions_mutex);
    if (!found) return;

    /* g_room_members 동기화 */
    for (i = 0; i < g_room_member_count; i++) {
        if (g_room_members[i].room_id == room_id &&
            strcmp(g_room_members[i].user_id, target_id) == 0) {
            g_room_members[i].is_admin = 0;
            break;
        }
    }

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_room_members(FILE_ROOM_MEMBERS);
    ReleaseMutex(g_file_mutex);
}

/* ROOM_MEMBERS_REQ|room_id
 * → ROOM_MEMBERS_RES|count:user_id:nick:is_admin:online_status;... */
static void handle_room_members(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    int room_id = payload ? atoi(payload) : 0;
    if (room_id == 0) return;

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0) { ReleaseMutex(g_sessions_mutex); return; }

    /* 멤버 여부 확인 */
    int j, is_member = 0;
    for (j = 0; j < g_rooms[idx].member_count; j++) {
        if (strcmp(g_rooms[idx].member_ids[j], sess->user_id) == 0) {
            is_member = 1; break;
        }
    }
    if (!is_member) { ReleaseMutex(g_sessions_mutex); return; }

    char items[MAX_PKT_SIZE];
    int  ioff = 0, cnt = 0;

    for (j = 0; j < g_rooms[idx].member_count && ioff < (int)sizeof(items) - 64; j++) {
        char *uid = g_rooms[idx].member_ids[j];
        int   adm = g_rooms[idx].admin_flags[j];

        int online_st = 0, k;
        for (k = 0; k < MAX_CLIENTS; k++) {
            if (g_sessions[k].active && strcmp(g_sessions[k].user_id, uid) == 0) {
                online_st = g_sessions[k].online_status;
                break;
            }
        }

        char nick[21];
        strncpy(nick, uid, 20); nick[20] = '\0';
        int u;
        for (u = 0; u < g_user_count; u++) {
            if (strcmp(g_users[u].id_str, uid) == 0) {
                strncpy(nick, g_users[u].nickname, 20);
                break;
            }
        }

        if (cnt > 0) items[ioff++] = ';';
        ioff += snprintf(items + ioff, sizeof(items) - ioff,
                         "%s:%s:%d:%d", uid, nick, adm, online_st);
        cnt++;
    }
    ReleaseMutex(g_sessions_mutex);
    items[ioff] = '\0';

    char buf[MAX_PKT_SIZE];
    int  off = snprintf(buf, sizeof(buf) - 2,
                        ROOM_MEMBERS_RES "|%d%s%s",
                        cnt, cnt > 0 ? ":" : "", items);
    buf[off++] = '\n'; buf[off] = '\0';
    send(sess->sock, buf, off, 0);
}

/* ROOM_SET_OPEN_NICK|room_id:new_nick  (new_nick content-last)
 * 오픈채팅 방에서 자신의 닉네임을 변경한다.
 * → ROOM_SET_OPEN_NICK_RES|0 (성공) or 1 (실패) */
static void handle_room_set_open_nick(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *room_id_s = strtok(payload, ":");
    char *new_nick  = strtok(NULL, "");
    if (!room_id_s || !new_nick) {
        send_packet(sess->sock, ROOM_SET_OPEN_NICK_RES "|1");
        return;
    }

    int n = (int)strlen(new_nick);
    while (n > 0 && (new_nick[n-1] == '\r' || new_nick[n-1] == '\n'))
        new_nick[--n] = '\0';
    if (n == 0 || n > 20) {
        send_packet(sess->sock, ROOM_SET_OPEN_NICK_RES "|1");
        return;
    }

    int room_id = atoi(room_id_s);
    int i;

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0 || !g_rooms[idx].info.is_open) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, ROOM_SET_OPEN_NICK_RES "|1");
        return;
    }

    int is_member = 0;
    for (i = 0; i < g_rooms[idx].member_count; i++) {
        if (strcmp(g_rooms[idx].member_ids[i], sess->user_id) == 0) {
            is_member = 1; break;
        }
    }
    ReleaseMutex(g_sessions_mutex);

    if (!is_member) {
        send_packet(sess->sock, ROOM_SET_OPEN_NICK_RES "|1");
        return;
    }

    /* g_room_members 업데이트 */
    for (i = 0; i < g_room_member_count; i++) {
        if (g_room_members[i].room_id == room_id &&
            strcmp(g_room_members[i].user_id, sess->user_id) == 0) {
            strncpy(g_room_members[i].open_nick, new_nick, 20);
            break;
        }
    }

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_room_members(FILE_ROOM_MEMBERS);
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, ROOM_SET_OPEN_NICK_RES "|0");
}

/* ROOM_DELETE|room_id  — 방장 전용
 * is_deleted=1 로 표시 후 ROOM_DELETED_NOTIFY|room_id 브로드캐스트. */
static void handle_room_delete(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    int room_id = payload ? atoi(payload) : 0;
    if (room_id == 0) return;

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int idx = find_room_idx(room_id);
    if (idx < 0 || strcmp(g_rooms[idx].info.owner_id, sess->user_id) != 0) {
        ReleaseMutex(g_sessions_mutex);
        return;
    }

    g_rooms[idx].info.is_deleted = 1;
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_rooms(FILE_ROOMS);
    ReleaseMutex(g_file_mutex);

    char buf[64];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         ROOM_DELETED_NOTIFY "|%d", room_id);
    if (mlen > 0 && mlen < (int)sizeof(buf) - 2) {
        buf[mlen++] = '\n'; buf[mlen] = '\0';
        broadcast_to_room(room_id, buf);
    }
}

/* ROOM_SEARCH|keyword  (content-last)
 * → ROOM_SEARCH_RES|count:room_id:name:member:max:has_pw:is_open:topic;... */
static void handle_room_search(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *kw = payload ? payload : "";
    int   n  = (int)strlen(kw);
    while (n > 0 && (kw[n-1] == '\r' || kw[n-1] == '\n'))
        kw[--n] = '\0';

    char items[MAX_PKT_SIZE];
    int  ioff = 0, cnt = 0, i;

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < g_room_count && ioff < (int)sizeof(items) - 128; i++) {
        if (g_rooms[i].info.is_deleted) continue;
        if (n > 0 &&
            strstr(g_rooms[i].info.name,  kw) == NULL &&
            strstr(g_rooms[i].info.topic, kw) == NULL) continue;

        int has_pw = (g_rooms[i].info.pw_hash[0] != '\0');
        if (cnt > 0) items[ioff++] = ';';
        ioff += snprintf(items + ioff, sizeof(items) - ioff,
                         "%d:%s:%d:%d:%d:%d:%s",
                         g_rooms[i].info.id, g_rooms[i].info.name,
                         g_rooms[i].member_count, g_rooms[i].info.max_members,
                         has_pw, g_rooms[i].info.is_open,
                         g_rooms[i].info.topic);
        cnt++;
    }
    ReleaseMutex(g_sessions_mutex);
    items[ioff] = '\0';

    char buf[MAX_PKT_SIZE];
    int  off = snprintf(buf, sizeof(buf) - 2,
                        ROOM_SEARCH_RES "|%d%s%s",
                        cnt, cnt > 0 ? ":" : "", items);
    buf[off++] = '\n'; buf[off] = '\0';
    send(sess->sock, buf, off, 0);
}

/* ROOM_MUTE_TOGGLE|room_id
 * 자신의 뮤트 상태 토글. → ROOM_MUTE_TOGGLE_RES|room_id:new_mute */
static void handle_room_mute_toggle(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    int room_id = payload ? atoi(payload) : 0;
    if (room_id == 0) return;

    int new_mute = 0, i;

    for (i = 0; i < g_room_member_count; i++) {
        if (g_room_members[i].room_id == room_id &&
            strcmp(g_room_members[i].user_id, sess->user_id) == 0) {
            g_room_members[i].is_muted = !g_room_members[i].is_muted;
            new_mute = g_room_members[i].is_muted;
            break;
        }
    }

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_room_members(FILE_ROOM_MEMBERS);
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, ROOM_MUTE_TOGGLE_RES "|%d:%d", room_id, new_mute);
}

/* TYPING_START|room_id — 방 멤버(송신자 제외)에게 TYPING_NOTIFY|room_id:nick:1 전송 */
static void handle_typing_start(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;
    char *room_id_s = strtok(payload, ":");
    if (!room_id_s) return;
    int pn = (int)strlen(room_id_s);
    while (pn > 0 && (room_id_s[pn-1] == '\r' || room_id_s[pn-1] == '\n'))
        room_id_s[--pn] = '\0';
    int room_id = atoi(room_id_s);
    if (room_id <= 0) return;

    char buf[256];
    int mlen = snprintf(buf, sizeof(buf) - 2,
                        TYPING_NOTIFY "|%d:%s:1", room_id, sess->nickname);
    if (mlen <= 0 || mlen >= (int)sizeof(buf) - 2) return;
    buf[mlen++] = '\n'; buf[mlen] = '\0';

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!g_sessions[i].active) continue;
        if (g_sessions[i].room_id != room_id) continue;
        if (strcmp(g_sessions[i].user_id, sess->user_id) == 0) continue;
        send(g_sessions[i].sock, buf, mlen, 0);
    }
    ReleaseMutex(g_sessions_mutex);
}

/* TYPING_STOP|room_id — 방 멤버(송신자 제외)에게 TYPING_NOTIFY|room_id:nick:0 전송 */
static void handle_typing_stop(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;
    char *room_id_s = strtok(payload, ":");
    if (!room_id_s) return;
    int pn = (int)strlen(room_id_s);
    while (pn > 0 && (room_id_s[pn-1] == '\r' || room_id_s[pn-1] == '\n'))
        room_id_s[--pn] = '\0';
    int room_id = atoi(room_id_s);
    if (room_id <= 0) return;

    char buf[256];
    int mlen = snprintf(buf, sizeof(buf) - 2,
                        TYPING_NOTIFY "|%d:%s:0", room_id, sess->nickname);
    if (mlen <= 0 || mlen >= (int)sizeof(buf) - 2) return;
    buf[mlen++] = '\n'; buf[mlen] = '\0';

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!g_sessions[i].active) continue;
        if (g_sessions[i].room_id != room_id) continue;
        if (strcmp(g_sessions[i].user_id, sess->user_id) == 0) continue;
        send(g_sessions[i].sock, buf, mlen, 0);
    }
    ReleaseMutex(g_sessions_mutex);
}

void room_init(void) {
    register_handler(ROOM_LIST_REQ,      handle_room_list);
    register_handler(ROOM_CREATE,        handle_room_create);
    register_handler(ROOM_JOIN,          handle_room_join);
    register_handler(ROOM_LEAVE,         handle_room_leave);
    register_handler(ROOM_MSG,           handle_room_msg);
    register_handler(ROOM_HISTORY_REQ,   handle_room_history);
    register_handler(ROOM_INVITE,        handle_room_invite);
    register_handler(ROOM_KICK,          handle_room_kick);
    register_handler(ROOM_SET_NOTICE,    handle_room_set_notice);
    register_handler(ROOM_GRANT_ADMIN,   handle_room_grant_admin);
    register_handler(ROOM_REVOKE_ADMIN,  handle_room_revoke_admin);
    register_handler(ROOM_MEMBERS_REQ,   handle_room_members);
    register_handler(ROOM_SET_OPEN_NICK, handle_room_set_open_nick);
    register_handler(ROOM_DELETE,        handle_room_delete);
    register_handler(ROOM_SEARCH,        handle_room_search);
    register_handler(ROOM_MUTE_TOGGLE,   handle_room_mute_toggle);
    register_handler(TYPING_START,       handle_typing_start);
    register_handler(TYPING_STOP,        handle_typing_stop);
}
