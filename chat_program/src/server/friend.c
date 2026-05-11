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

/* receiver_id 가 sender_id 를 차단했는지 확인.
 * g_sessions_mutex 없이 읽기 전용 접근 — 호출 전 mutex 보유 불필요. */
/* 받는 사람이 보낸 사람을 차단했는지 확인한다. */
int is_blocked_by(const char *receiver_id, const char *sender_id) {
    int i;
    for (i = 0; i < g_friend_count; i++) {
        if (g_friends[i].status == FRIEND_BLOCKED_S &&
            strcmp(g_friends[i].user_id,   receiver_id) == 0 &&
            strcmp(g_friends[i].friend_id, sender_id)   == 0)
            return 1;
    }
    return 0;
}

/* 두 유저 사이의 기존 FriendRecord 포인터 반환. 없으면 NULL.
 * MUTEX: 호출자가 g_sessions_mutex 를 보유해야 한다. */
static FriendRecord *find_friend_record(const char *a, const char *b) {
    int i;
    for (i = 0; i < g_friend_count; i++) {
        if ((strcmp(g_friends[i].user_id,   a) == 0 && strcmp(g_friends[i].friend_id, b) == 0) ||
            (strcmp(g_friends[i].user_id,   b) == 0 && strcmp(g_friends[i].friend_id, a) == 0))
            return &g_friends[i];
    }
    return NULL;
}

/* 친구 관계를 차단 상태로 바꾼다. */
int friend_block_user(const char *user_id, const char *target_id) {
    FriendRecord *fr = find_friend_record(user_id, target_id);
    if (fr) {
        if (fr->status != FRIEND_BLOCKED_S)
            fr->status_before_block = fr->status;
        strncpy(fr->user_id,   user_id,   20); fr->user_id[20] = '\0';
        strncpy(fr->friend_id, target_id, 20); fr->friend_id[20] = '\0';
        fr->status = FRIEND_BLOCKED_S;
        return 1;
    }

    if (g_friend_count >= MAX_FRIENDS)
        return 0;

    fr = &g_friends[g_friend_count];
    memset(fr, 0, sizeof(*fr));
    fr->id = g_next_friend_id++;
    strncpy(fr->user_id,   user_id,   20); fr->user_id[20] = '\0';
    strncpy(fr->friend_id, target_id, 20); fr->friend_id[20] = '\0';
    fr->status = FRIEND_BLOCKED_S;
    fr->status_before_block = -1;
    get_current_timestamp(fr->created_at);
    g_friend_count++;
    return 1;
}

/* 차단 상태를 풀고 이전 친구 상태로 되돌린다. */
int friend_unblock_user(const char *user_id, const char *target_id) {
    int i, k;
    for (i = 0; i < g_friend_count; i++) {
        FriendRecord *fr = &g_friends[i];
        if (fr->status == FRIEND_BLOCKED_S &&
            strcmp(fr->user_id, user_id) == 0 &&
            strcmp(fr->friend_id, target_id) == 0) {
            if (fr->status_before_block == FRIEND_PENDING ||
                fr->status_before_block == FRIEND_ACCEPTED) {
                fr->status = fr->status_before_block;
                fr->status_before_block = -1;
            } else {
                for (k = i; k < g_friend_count - 1; k++)
                    g_friends[k] = g_friends[k+1];
                g_friend_count--;
            }
            return 1;
        }
    }
    return 0;
}

/* FRIEND_ADD_REQ|target_id
 * → FRIEND_ADD_RES|code; 대상이 온라인이면 FRIEND_REQUEST_NOTIFY 전송 */
/* 친구 추가 요청을 만들고 상대에게 알린다. */
static void handle_friend_add(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *target_id = payload;
    if (!target_id) return;
    int n = (int)strlen(target_id);
    while (n > 0 && (target_id[n-1] == '\r' || target_id[n-1] == '\n'))
        target_id[--n] = '\0';

    if (n == 0 || strcmp(target_id, sess->user_id) == 0) {
        send_packet(sess->sock, FRIEND_ADD_RES "|%d", FRIEND_NOT_FOUND);
        return;
    }

    if (!find_user_by_id(target_id)) {
        send_packet(sess->sock, FRIEND_ADD_RES "|%d", FRIEND_NOT_FOUND);
        return;
    }

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    FriendRecord *existing = find_friend_record(sess->user_id, target_id);
    if (existing) {
        int st = existing->status;
        ReleaseMutex(g_sessions_mutex);
        if (st == FRIEND_BLOCKED_S)
            send_packet(sess->sock, FRIEND_ADD_RES "|%d", FRIEND_BLOCKED);
        else
            send_packet(sess->sock, FRIEND_ADD_RES "|%d", FRIEND_ALREADY);
        return;
    }
    if (g_friend_count >= MAX_FRIENDS) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, FRIEND_ADD_RES "|%d", FRIEND_NOT_FOUND);
        return;
    }
    FriendRecord *fr = &g_friends[g_friend_count];
    memset(fr, 0, sizeof(*fr));
    fr->id = g_next_friend_id++;
    strncpy(fr->user_id,    sess->user_id, 20);
    strncpy(fr->friend_id,  target_id,     20);
    fr->status = FRIEND_PENDING;
    fr->status_before_block = -1;
    get_current_timestamp(fr->created_at);
    g_friend_count++;
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    append_friend(FILE_FRIENDS, fr);
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, FRIEND_ADD_RES "|%d", FRIEND_SENT);

    /* 대상이 온라인이면 친구 요청 알림 */
    char nick[21];
    get_nickname(sess->user_id, nick);
    char buf[256];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         FRIEND_REQUEST_NOTIFY "|%s:%s", sess->user_id, nick);
    buf[mlen++] = '\n'; buf[mlen] = '\0';
    send_to_user(target_id, buf);
}

/* FRIEND_ACCEPT|requester_id → 요청자에게 FRIEND_ACCEPT_NOTIFY 전송 */
/* 받은 친구 요청을 수락 처리한다. */
static void handle_friend_accept(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *req_id = payload;
    if (!req_id) return;
    int n = (int)strlen(req_id);
    while (n > 0 && (req_id[n-1] == '\r' || req_id[n-1] == '\n'))
        req_id[--n] = '\0';

    /* 방향: user_id=requester, friend_id=sess (accepter) */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int i, found = 0;
    for (i = 0; i < g_friend_count; i++) {
        if (strcmp(g_friends[i].user_id,   req_id)        == 0 &&
            strcmp(g_friends[i].friend_id, sess->user_id) == 0 &&
            g_friends[i].status == FRIEND_PENDING) {
            g_friends[i].status = FRIEND_ACCEPTED;
            found = 1; break;
        }
    }
    ReleaseMutex(g_sessions_mutex);
    if (!found) return;

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_friends(FILE_FRIENDS);
    ReleaseMutex(g_file_mutex);

    char nick[21];
    get_nickname(sess->user_id, nick);
    char buf[256];
    int  mlen = snprintf(buf, sizeof(buf) - 2,
                         FRIEND_ACCEPT_NOTIFY "|%s:%s", sess->user_id, nick);
    buf[mlen++] = '\n'; buf[mlen] = '\0';
    send_to_user(req_id, buf);
}

/* FRIEND_REJECT|requester_id — pending 레코드 삭제 */
/* 받은 친구 요청을 거절 처리한다. */
static void handle_friend_reject(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *req_id = payload;
    if (!req_id) return;
    int n = (int)strlen(req_id);
    while (n > 0 && (req_id[n-1] == '\r' || req_id[n-1] == '\n'))
        req_id[--n] = '\0';

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int i, k;
    for (i = 0; i < g_friend_count; i++) {
        if (strcmp(g_friends[i].user_id,   req_id)        == 0 &&
            strcmp(g_friends[i].friend_id, sess->user_id) == 0 &&
            g_friends[i].status == FRIEND_PENDING) {
            for (k = i; k < g_friend_count - 1; k++)
                g_friends[k] = g_friends[k+1];
            g_friend_count--;
            break;
        }
    }
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_friends(FILE_FRIENDS);
    ReleaseMutex(g_file_mutex);
}

/* FRIEND_DELETE|friend_id — 수락된 친구 관계 제거 */
/* 친구 관계를 목록에서 제거한다. */
static void handle_friend_delete(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *friend_id = payload;
    if (!friend_id) return;
    int n = (int)strlen(friend_id);
    while (n > 0 && (friend_id[n-1] == '\r' || friend_id[n-1] == '\n'))
        friend_id[--n] = '\0';

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int i, k;
    for (i = 0; i < g_friend_count; i++) {
        int is_pair = (strcmp(g_friends[i].user_id,   sess->user_id) == 0 &&
                       strcmp(g_friends[i].friend_id, friend_id)     == 0) ||
                      (strcmp(g_friends[i].user_id,   friend_id)     == 0 &&
                       strcmp(g_friends[i].friend_id, sess->user_id) == 0);
        if (is_pair && g_friends[i].status == FRIEND_ACCEPTED) {
            for (k = i; k < g_friend_count - 1; k++)
                g_friends[k] = g_friends[k+1];
            g_friend_count--;
            break;
        }
    }
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_friends(FILE_FRIENDS);
    ReleaseMutex(g_file_mutex);
}

/* FRIEND_BLOCK|target_id — 기존 레코드를 차단 상태로 변경하거나 새로 생성 */
/* 선택한 사용자를 차단한다. */
static void handle_friend_block(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *target_id = payload;
    if (!target_id) return;
    int n = (int)strlen(target_id);
    while (n > 0 && (target_id[n-1] == '\r' || target_id[n-1] == '\n'))
        target_id[--n] = '\0';

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    FriendRecord *fr = find_friend_record(sess->user_id, target_id);
    if (fr) {
        if (fr->status != FRIEND_BLOCKED_S)
            fr->status_before_block = fr->status;
        /* 방향 재설정: 차단자=user_id, 피차단=friend_id */
        strncpy(fr->user_id,   sess->user_id, 20);
        strncpy(fr->friend_id, target_id,     20);
        fr->status = FRIEND_BLOCKED_S;
    } else if (g_friend_count < MAX_FRIENDS) {
        fr = &g_friends[g_friend_count];
        memset(fr, 0, sizeof(*fr));
        fr->id = g_next_friend_id++;
        strncpy(fr->user_id,   sess->user_id, 20);
        strncpy(fr->friend_id, target_id,     20);
        fr->status = FRIEND_BLOCKED_S;
        fr->status_before_block = -1;
        get_current_timestamp(fr->created_at);
        g_friend_count++;
    }
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_friends(FILE_FRIENDS);
    ReleaseMutex(g_file_mutex);
}

/* FRIEND_UNBLOCK|target_id */
/* 선택한 사용자의 차단을 해제한다. */
static void handle_friend_unblock(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *target_id = payload;
    if (!target_id) return;
    int n = (int)strlen(target_id);
    while (n > 0 && (target_id[n-1] == '\r' || target_id[n-1] == '\n'))
        target_id[--n] = '\0';

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    friend_unblock_user(sess->user_id, target_id);
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_friends(FILE_FRIENDS);
    ReleaseMutex(g_file_mutex);
}

/* FRIEND_LIST_REQ|
 * → FRIEND_LIST_RES|count:friend_id:nick:friend_status:status_msg;... (status_msg content-last) */
/* 현재 친구 목록을 만들어 클라이언트에 보낸다. */
static void handle_friend_list(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;

    /* 아이템을 먼저 임시 버퍼에 모은 후 count 를 앞에 붙인다 */
    char items[MAX_PKT_SIZE];
    int  ioff = 0, cnt = 0, i;

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < g_friend_count && ioff < (int)sizeof(items) - 64; i++) {
        FriendRecord *fr = &g_friends[i];
        const char *other = NULL;
        if (fr->status == FRIEND_BLOCKED_S) {
            if (strcmp(fr->user_id, sess->user_id) == 0) other = fr->friend_id;
            else continue;
        }
        else if (strcmp(fr->user_id,   sess->user_id) == 0) other = fr->friend_id;
        else if (strcmp(fr->friend_id, sess->user_id) == 0) other = fr->user_id;
        else continue;

        /* mutex 보유 중이므로 직접 닉네임/상태메시지 조회 */
        char nick[21];
        char status_msg[101];
        int  u;
        strncpy(nick, other, 20); nick[20] = '\0';
        status_msg[0] = '\0';
        for (u = 0; u < g_user_count; u++) {
            if (strcmp(g_users[u].id_str, other) == 0) {
                strncpy(nick, g_users[u].nickname, 20);
                strncpy(status_msg, g_users[u].status_msg, 100);
                break;
            }
        }
        /* 온라인 상태는 FRIEND_STATUS_CHANGE 로 별도 통보됨 */

        if (cnt > 0) items[ioff++] = ';';
        ioff += snprintf(items + ioff, sizeof(items) - ioff,
                         "%s:%s:%d:%s", other, nick, fr->status, status_msg);
        cnt++;
    }
    ReleaseMutex(g_sessions_mutex);
    items[ioff] = '\0';

    char buf[MAX_PKT_SIZE];
    int  off = snprintf(buf, sizeof(buf) - 2,
                        FRIEND_LIST_RES "|%d%s%s",
                        cnt, cnt > 0 ? ":" : "", items);
    buf[off++] = '\n'; buf[off] = '\0';
    send(sess->sock, buf, off, 0);
}

/* USER_SEARCH_REQ|keyword
 * → USER_SEARCH_RES|count:user_id:nick;... */
/* 닉네임이나 아이디로 사용자를 검색한다. */
static void handle_user_search(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *kw = payload ? payload : "";
    int n = (int)strlen(kw);
    while (n > 0 && (kw[n-1] == '\r' || kw[n-1] == '\n'))
        kw[--n] = '\0';

    char items[MAX_PKT_SIZE];
    int  ioff = 0, cnt = 0, i;

    for (i = 0; i < g_user_count && ioff < (int)sizeof(items) - 64; i++) {
        if (strcmp(g_users[i].id_str, sess->user_id) == 0) continue;
        if (n > 0 &&
            strstr(g_users[i].id_str,   kw) == NULL &&
            strstr(g_users[i].nickname, kw) == NULL) continue;

        if (cnt > 0) items[ioff++] = ';';
        ioff += snprintf(items + ioff, sizeof(items) - ioff,
                         "%s:%s", g_users[i].id_str, g_users[i].nickname);
        cnt++;
    }
    items[ioff] = '\0';

    char buf[MAX_PKT_SIZE];
    int  off = snprintf(buf, sizeof(buf) - 2,
                        USER_SEARCH_RES "|%d%s%s",
                        cnt, cnt > 0 ? ":" : "", items);
    buf[off++] = '\n'; buf[off] = '\0';
    send(sess->sock, buf, off, 0);
}

/* 친구 관련 패킷 처리 함수를 등록한다. */
void friend_init(void) {
    register_handler(FRIEND_ADD_REQ,  handle_friend_add);
    register_handler(FRIEND_ACCEPT,   handle_friend_accept);
    register_handler(FRIEND_REJECT,   handle_friend_reject);
    register_handler(FRIEND_DELETE,   handle_friend_delete);
    register_handler(FRIEND_BLOCK,    handle_friend_block);
    register_handler(FRIEND_UNBLOCK,  handle_friend_unblock);
    register_handler(FRIEND_LIST_REQ, handle_friend_list);
    register_handler(USER_SEARCH_REQ, handle_user_search);
}
