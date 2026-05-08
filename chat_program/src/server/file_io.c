#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../common/protocol.h"
#include "../common/types.h"
#include "globals.h"
#include "file_io.h"

/* ---------------------------------------------------------------
 * split_line: "//"-구분자로 line 을 fields[] 에 분리한다.
 * - 마지막 필드(index max_fields-1)는 나머지를 모두 포함 (content-last).
 * - 빈 필드는 "////" 패턴으로 표현되며 "" 으로 파싱된다.
 * - 호출 전 line 끝의 '\r', '\n' 을 제거한다.
 * 반환값: 파싱된 실제 필드 수
 * --------------------------------------------------------------- */
static int split_line(char *line, char **fields, int max_fields) {
    size_t len;
    int    n   = 0;
    char  *p   = line;
    char  *sep;

    /* 끝 개행 제거 */
    len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        line[--len] = '\0';

    if (len == 0) { fields[0] = line; return 1; }

    while (n < max_fields - 1) {
        sep = strstr(p, "//");
        if (!sep) break;
        fields[n] = p;
        *sep = '\0';
        p    = sep + 2;
        n++;
    }
    fields[n] = p;   /* 마지막 필드 (content-last) */
    return n + 1;
}

/* ---------------------------------------------------------------
 * users.txt
 * 포맷: id//pw_hash//nickname//status_msg//online_status//is_admin//last_seen//created_at
 * --------------------------------------------------------------- */
int load_users(const char *path) {
    FILE  *fp;
    char   line[512];
    char  *fields[10];
    int    n;

    fp = fopen(path, "r");
    if (!fp) return 0;   /* 파일 없음 → 빈 배열로 시작 (정상) */

    g_user_count = 0;
    while (fgets(line, sizeof(line), fp) && g_user_count < MAX_CLIENTS) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 8);
        if (n < 8) continue;

        UserRecord *u = &g_users[g_user_count];
        memset(u, 0, sizeof(*u));
        u->id = g_user_count + 1;                        /* 내부 순번 */
        strncpy(u->id_str,        fields[0], 20);
        strncpy(u->pw_hash,       fields[1], 64);
        strncpy(u->nickname,      fields[2], 20);
        strncpy(u->status_msg,    fields[3], 100);
        u->online_status = atoi(fields[4]);
        /* fields[5] = is_admin (struct에 없음, 무시) */
        strncpy(u->last_seen,     fields[6], 19);
        strncpy(u->created_at,    fields[7], 19);
        g_user_count++;
    }
    fclose(fp);
    return g_user_count;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
void save_users(const char *path) {
    FILE *fp;
    int   i;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "w");
    if (!fp) return;

    for (i = 0; i < g_user_count; i++) {
        UserRecord *u = &g_users[i];
        fprintf(fp, "%s//%s//%s//%s//%d//0//%s//%s\n",
                u->id_str, u->pw_hash, u->nickname,
                u->status_msg, u->online_status,
                u->last_seen, u->created_at);
    }
    fclose(fp);
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
int append_user(const char *path, const UserRecord *u) {
    FILE *fp;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "a");
    if (!fp) return -1;

    fprintf(fp, "%s//%s//%s//%s//%d//0//%s//%s\n",
            u->id_str, u->pw_hash, u->nickname,
            u->status_msg, u->online_status,
            u->last_seen, u->created_at);
    fclose(fp);
    return 0;
}

/* ---------------------------------------------------------------
 * rooms.txt
 * 포맷: id//name//topic//pw_hash//max_users//owner_id//notice//is_open//pinned_msg_id//created_at
 * --------------------------------------------------------------- */
int load_rooms(const char *path) {
    FILE  *fp;
    char   line[512];
    char  *fields[11];
    int    n;

    fp = fopen(path, "r");
    if (!fp) return 0;

    g_room_count = 0;
    while (fgets(line, sizeof(line), fp) && g_room_count < MAX_ROOMS) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 10);
        if (n < 10) continue;

        RoomInfo *ri = &g_rooms[g_room_count];
        memset(ri, 0, sizeof(*ri));

        ri->info.id          = atoi(fields[0]);
        strncpy(ri->info.name,      fields[1], 30);
        strncpy(ri->info.topic,     fields[2], 100);
        strncpy(ri->info.pw_hash,   fields[3], 64);
        ri->info.max_members = atoi(fields[4]);
        strncpy(ri->info.owner_id,  fields[5], 20);
        strncpy(ri->info.notice,    fields[6], 200);
        ri->info.is_open      = atoi(fields[7]);
        ri->info.pinned_msg_id= atoi(fields[8]);
        strncpy(ri->info.created_at,fields[9], 19);
        ri->info.is_deleted   = 0;
        ri->member_count      = 0;
        g_room_count++;
    }
    fclose(fp);
    return g_room_count;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
void save_rooms(const char *path) {
    FILE *fp;
    int   i;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "w");
    if (!fp) return;

    for (i = 0; i < g_room_count; i++) {
        RoomRecord *r = &g_rooms[i].info;
        if (r->is_deleted) continue;
        fprintf(fp, "%d//%s//%s//%s//%d//%s//%s//%d//%d//%s\n",
                r->id, r->name, r->topic, r->pw_hash,
                r->max_members, r->owner_id, r->notice,
                r->is_open, r->pinned_msg_id, r->created_at);
    }
    fclose(fp);
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
int append_room(const char *path, const RoomRecord *r) {
    FILE *fp;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "a");
    if (!fp) return -1;

    fprintf(fp, "%d//%s//%s//%s//%d//%s//%s//%d//%d//%s\n",
            r->id, r->name, r->topic, r->pw_hash,
            r->max_members, r->owner_id, r->notice,
            r->is_open, r->pinned_msg_id, r->created_at);
    fclose(fp);
    return 0;
}

/* ---------------------------------------------------------------
 * room_members.txt
 * 포맷: room_id//user_id//open_nick//is_admin//is_muted//joined_at
 * 로드 시 g_rooms[].member_ids / member_count 도 갱신한다.
 * --------------------------------------------------------------- */
int load_room_members(const char *path) {
    FILE  *fp;
    char   line[256];
    char  *fields[7];
    int    n, count = 0;
    int    i;

    fp = fopen(path, "r");
    if (!fp) return 0;

    g_room_member_count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 6);
        if (n < 6) continue;

        int   room_id  = atoi(fields[0]);
        char *user_id  = fields[1];
        char *open_nick = fields[2];
        int   is_admin = atoi(fields[3]);
        int   is_muted = atoi(fields[4]);
        char *joined_at = fields[5];

        /* g_room_members 전체 레코드 저장 */
        if (g_room_member_count < MAX_ROOM_MEMBER_RECORDS) {
            RoomMemberRecord *m = &g_room_members[g_room_member_count];
            memset(m, 0, sizeof(*m));
            m->room_id  = room_id;
            strncpy(m->user_id,   user_id,   20);
            strncpy(m->open_nick, open_nick,  20);
            m->is_admin = is_admin;
            m->is_muted = is_muted;
            strncpy(m->joined_at, joined_at, 19);
            g_room_member_count++;
        }

        /* 해당 방을 g_rooms[] 에서 찾아 member_ids / admin_flags 에 추가 */
        for (i = 0; i < g_room_count; i++) {
            if (g_rooms[i].info.id == room_id) {
                int mc = g_rooms[i].member_count;
                if (mc < MAX_ROOM_MEMBERS) {
                    strncpy(g_rooms[i].member_ids[mc], user_id, 20);
                    g_rooms[i].member_ids[mc][20] = '\0';
                    g_rooms[i].admin_flags[mc] = is_admin;
                    g_rooms[i].member_count++;
                }
                break;
            }
        }
        count++;
    }
    fclose(fp);
    return count;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
void save_room_members(const char *path) {
    FILE *fp;
    int   i, j;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "w");
    if (!fp) return;

    for (i = 0; i < g_room_member_count; i++) {
        RoomMemberRecord *m = &g_room_members[i];
        /* 삭제된 방의 멤버는 저장 제외 */
        int room_deleted = 0;
        for (j = 0; j < g_room_count; j++) {
            if (g_rooms[j].info.id == m->room_id) {
                if (g_rooms[j].info.is_deleted) room_deleted = 1;
                break;
            }
        }
        if (room_deleted) continue;
        fprintf(fp, "%d//%s//%s//%d//%d//%s\n",
                m->room_id, m->user_id, m->open_nick,
                m->is_admin, m->is_muted, m->joined_at);
    }
    fclose(fp);
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
int append_room_member(const char *path, const RoomMemberRecord *m) {
    FILE *fp;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "a");
    if (!fp) return -1;

    fprintf(fp, "%d//%s//%s//%d//%d//%s\n",
            m->room_id, m->user_id, m->open_nick,
            m->is_admin, m->is_muted, m->joined_at);
    fclose(fp);
    return 0;
}

/* ---------------------------------------------------------------
 * messages.txt
 * 포맷 (content-last):
 * id//room_id//from_id//to_id//reply_to//msg_type//is_deleted//created_at//edited_at//content
 *
 * P0 에서는 메시지 전체를 인메모리에 저장하지 않는다.
 * g_next_msg_id 만 갱신한다 (T5 에서 g_messages 배열 추가 시 개선).
 * --------------------------------------------------------------- */
int load_messages(const char *path) {
    FILE  *fp;
    char   line[MAX_PKT_SIZE + 256];
    char  *fields[11];
    int    n, count = 0;
    int    msg_id;

    fp = fopen(path, "r");
    if (!fp) return 0;

    g_next_msg_id = 1;
    g_msg_count   = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 10);
        if (n < 1) continue;

        msg_id = atoi(fields[0]);
        if (msg_id >= g_next_msg_id)
            g_next_msg_id = msg_id + 1;

        /* g_messages[] 에 채운다 (MAX_MSG_HISTORY 초과분은 ID 추적만 유지) */
        if (n >= 10 && g_msg_count < MAX_MSG_HISTORY) {
            MessageRecord *m = &g_messages[g_msg_count];
            memset(m, 0, sizeof(*m));
            m->id          = atoi(fields[0]);
            m->room_id     = atoi(fields[1]);
            strncpy(m->from_id,    fields[2], 20);
            strncpy(m->to_id,      fields[3], 20);
            m->reply_to_id = atoi(fields[4]);
            m->msg_type    = atoi(fields[5]);
            m->is_deleted  = atoi(fields[6]);
            strncpy(m->created_at, fields[7], 19);
            strncpy(m->edited_at,  fields[8], 19);
            strncpy(m->content,    fields[9], MAX_PKT_SIZE - 1);
            g_msg_count++;
        }
        count++;
    }
    fclose(fp);
    return count;
}

/* ---------------------------------------------------------------
 * friends.txt
 * 포맷: id//user_id//friend_id//status//created_at
 * --------------------------------------------------------------- */
int load_friends(const char *path) {
    FILE  *fp;
    char   line[256];
    char  *fields[6];
    int    n;

    fp = fopen(path, "r");
    if (!fp) return 0;

    g_friend_count = 0;
    while (fgets(line, sizeof(line), fp) && g_friend_count < MAX_FRIENDS) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 5);
        if (n < 5) continue;

        FriendRecord *fr = &g_friends[g_friend_count];
        memset(fr, 0, sizeof(*fr));
        fr->id = atoi(fields[0]);
        strncpy(fr->user_id,    fields[1], 20);
        strncpy(fr->friend_id,  fields[2], 20);
        fr->status = atoi(fields[3]);
        strncpy(fr->created_at, fields[4], 19);
        g_friend_count++;
    }
    fclose(fp);
    return g_friend_count;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
void save_friends(const char *path) {
    FILE *fp;
    int   i;

    fp = fopen(path, "w");
    if (!fp) return;

    for (i = 0; i < g_friend_count; i++) {
        FriendRecord *fr = &g_friends[i];
        fprintf(fp, "%d//%s//%s//%d//%s\n",
                fr->id, fr->user_id, fr->friend_id,
                fr->status, fr->created_at);
    }
    fclose(fp);
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
int append_friend(const char *path, const FriendRecord *f) {
    FILE *fp;

    fp = fopen(path, "a");
    if (!fp) return -1;

    fprintf(fp, "%d//%s//%s//%d//%s\n",
            f->id, f->user_id, f->friend_id,
            f->status, f->created_at);
    fclose(fp);
    return 0;
}

/* ---------------------------------------------------------------
 * dm_reads.txt
 * 포맷: msg_id//reader_id//read_at
 * --------------------------------------------------------------- */
int load_dm_reads(const char *path) {
    FILE  *fp;
    char   line[256];
    char  *fields[4];
    int    n;

    fp = fopen(path, "r");
    if (!fp) return 0;

    g_dm_read_count = 0;
    while (fgets(line, sizeof(line), fp) && g_dm_read_count < MAX_DM_READS) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 3);
        if (n < 3) continue;

        DmReadRecord *r = &g_dm_reads[g_dm_read_count];
        memset(r, 0, sizeof(*r));
        r->msg_id = atoi(fields[0]);
        strncpy(r->reader_id, fields[1], 20);
        strncpy(r->read_at,   fields[2], 19);
        g_dm_read_count++;
    }
    fclose(fp);
    return g_dm_read_count;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
void save_dm_reads(const char *path) {
    FILE *fp;
    int   i;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "w");
    if (!fp) return;

    for (i = 0; i < g_dm_read_count; i++) {
        DmReadRecord *r = &g_dm_reads[i];
        fprintf(fp, "%d//%s//%s\n",
                r->msg_id, r->reader_id, r->read_at);
    }
    fclose(fp);
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
int append_dm_read(const char *path, const DmReadRecord *r) {
    FILE *fp;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "a");
    if (!fp) return -1;

    fprintf(fp, "%d//%s//%s\n",
            r->msg_id, r->reader_id, r->read_at);
    fclose(fp);
    return 0;
}

/* ---------------------------------------------------------------
 * room_invites.txt
 * 포맷: id//room_id//inviter_id//invitee_id//status//created_at
 * --------------------------------------------------------------- */
int load_room_invites(const char *path) {
    FILE  *fp;
    char   line[256];
    char  *fields[7];
    int    n;

    fp = fopen(path, "r");
    if (!fp) return 0;

    g_room_invite_count = 0;
    while (fgets(line, sizeof(line), fp) && g_room_invite_count < MAX_INVITES) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 6);
        if (n < 6) continue;

        RoomInviteRecord *r = &g_room_invites[g_room_invite_count];
        memset(r, 0, sizeof(*r));
        r->id        = atoi(fields[0]);
        r->room_id   = atoi(fields[1]);
        strncpy(r->inviter_id, fields[2], 20);
        strncpy(r->invitee_id, fields[3], 20);
        r->status    = atoi(fields[4]);
        strncpy(r->created_at, fields[5], 19);
        g_room_invite_count++;
    }
    fclose(fp);
    return g_room_invite_count;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
void save_room_invites(const char *path) {
    FILE *fp;
    int   i;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "w");
    if (!fp) return;

    for (i = 0; i < g_room_invite_count; i++) {
        RoomInviteRecord *r = &g_room_invites[i];
        fprintf(fp, "%d//%d//%s//%s//%d//%s\n",
                r->id, r->room_id, r->inviter_id,
                r->invitee_id, r->status, r->created_at);
    }
    fclose(fp);
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
int append_room_invite(const char *path, const RoomInviteRecord *r) {
    FILE *fp;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "a");
    if (!fp) return -1;

    fprintf(fp, "%d//%d//%s//%s//%d//%s\n",
            r->id, r->room_id, r->inviter_id,
            r->invitee_id, r->status, r->created_at);
    fclose(fp);
    return 0;
}

/* ---------------------------------------------------------------
 * user_settings.txt
 * 포맷: user_id//msg_color//nick_color//theme//ts_format//dnd//welcome_shown
 * --------------------------------------------------------------- */
int load_user_settings(const char *path) {
    FILE  *fp;
    char   line[256];
    char  *fields[8];
    int    n;

    fp = fopen(path, "r");
    if (!fp) return 0;

    g_user_settings_count = 0;
    while (fgets(line, sizeof(line), fp) && g_user_settings_count < MAX_USERS) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 7);
        if (n < 7) continue;

        UserSettingsRecord *s = &g_user_settings[g_user_settings_count];
        memset(s, 0, sizeof(*s));
        strncpy(s->user_id,    fields[0], 20);
        strncpy(s->msg_color,  fields[1], 15);
        strncpy(s->nick_color, fields[2], 15);
        strncpy(s->theme,      fields[3], 10);
        s->ts_format     = atoi(fields[4]);
        s->dnd           = atoi(fields[5]);
        s->welcome_shown = atoi(fields[6]);
        g_user_settings_count++;
    }
    fclose(fp);
    return g_user_settings_count;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
void save_user_settings(const char *path) {
    FILE *fp;
    int   i;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "w");
    if (!fp) return;

    for (i = 0; i < g_user_settings_count; i++) {
        UserSettingsRecord *s = &g_user_settings[i];
        fprintf(fp, "%s//%s//%s//%s//%d//%d//%d\n",
                s->user_id, s->msg_color, s->nick_color,
                s->theme, s->ts_format, s->dnd, s->welcome_shown);
    }
    fclose(fp);
}

/* 호출자는 g_file_mutex 를 보유해야 한다.
 * user_id 가 이미 존재하면 갱신, 없으면 append. */
int upsert_user_settings(const char *path, const UserSettingsRecord *s) {
    int i;

    /* MUTEX: g_file_mutex (caller) */
    for (i = 0; i < g_user_settings_count; i++) {
        if (strcmp(g_user_settings[i].user_id, s->user_id) == 0) {
            g_user_settings[i] = *s;
            save_user_settings(path);
            return 0;
        }
    }
    /* 신규 */
    if (g_user_settings_count < MAX_USERS) {
        g_user_settings[g_user_settings_count++] = *s;
    }
    {
        FILE *fp = fopen(path, "a");
        if (!fp) return -1;
        fprintf(fp, "%s//%s//%s//%s//%d//%d//%d\n",
                s->user_id, s->msg_color, s->nick_color,
                s->theme, s->ts_format, s->dnd, s->welcome_shown);
        fclose(fp);
    }
    return 0;
}

/* ---------------------------------------------------------------
 * room_reads.txt
 * 포맷: room_id//user_id//last_read_msg_id//read_at
 * --------------------------------------------------------------- */
int load_room_reads(const char *path) {
    FILE  *fp;
    char   line[256];
    char  *fields[5];
    int    n;

    fp = fopen(path, "r");
    if (!fp) return 0;

    g_room_read_count = 0;
    while (fgets(line, sizeof(line), fp) && g_room_read_count < MAX_ROOM_READS) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
        n = split_line(line, fields, 4);
        if (n < 4) continue;

        RoomReadRecord *r = &g_room_reads[g_room_read_count];
        memset(r, 0, sizeof(*r));
        r->room_id          = atoi(fields[0]);
        strncpy(r->user_id, fields[1], 20);
        r->last_read_msg_id = atoi(fields[2]);
        strncpy(r->read_at, fields[3], 19);
        g_room_read_count++;
    }
    fclose(fp);
    return g_room_read_count;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
void save_room_reads(const char *path) {
    FILE *fp;
    int   i;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "w");
    if (!fp) return;

    for (i = 0; i < g_room_read_count; i++) {
        RoomReadRecord *r = &g_room_reads[i];
        fprintf(fp, "%d//%s//%d//%s\n",
                r->room_id, r->user_id,
                r->last_read_msg_id, r->read_at);
    }
    fclose(fp);
}

/* 호출자는 g_file_mutex 를 보유해야 한다.
 * room_id + user_id 조합이 존재하면 갱신, 없으면 신규 추가 후 파일 재작성. */
int update_room_read(const char *path, int room_id, const char *user_id,
                     int msg_id, const char *read_at) {
    int i;

    /* MUTEX: g_file_mutex (caller) */
    for (i = 0; i < g_room_read_count; i++) {
        if (g_room_reads[i].room_id == room_id &&
            strcmp(g_room_reads[i].user_id, user_id) == 0) {
            g_room_reads[i].last_read_msg_id = msg_id;
            strncpy(g_room_reads[i].read_at, read_at, 19);
            save_room_reads(path);
            return 0;
        }
    }
    /* 신규 항목 */
    if (g_room_read_count < MAX_ROOM_READS) {
        RoomReadRecord *r = &g_room_reads[g_room_read_count++];
        memset(r, 0, sizeof(*r));
        r->room_id          = room_id;
        strncpy(r->user_id, user_id, 20);
        r->last_read_msg_id = msg_id;
        strncpy(r->read_at, read_at, 19);
    }
    save_room_reads(path);
    return 0;
}

/* 호출자는 g_file_mutex 를 보유해야 한다. */
int append_message(const char *path, const MessageRecord *m) {
    FILE *fp;

    /* MUTEX: g_file_mutex (caller) */
    fp = fopen(path, "a");
    if (!fp) return -1;

    /* content-last: content 는 반드시 마지막 필드 */
    fprintf(fp, "%d//%d//%s//%s//%d//%d//%d//%s//%s//%s\n",
            m->id, m->room_id, m->from_id, m->to_id,
            m->reply_to_id, m->msg_type, m->is_deleted,
            m->created_at, m->edited_at, m->content);
    fclose(fp);
    return 0;
}
