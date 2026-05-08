#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "packet.h"
#include "chat_tui.h"

/* ── 리스트 캐시 전역 정의 ──────────────────────────────── */
FriendEntry   g_friend_list[MAX_FRIEND_LIST];
int           g_friend_count = 0;
DmPartnerEntry g_dm_list[MAX_DM_LIST];
int           g_dm_count = 0;

/* ─────────────────────────────────────────────────────────
 * display_chat_message
 * g_console_mutex를 획득한 뒤 콘솔에 메시지를 출력한다.
 * ───────────────────────────────────────────────────────── */
void display_chat_message(const char *from_nick, const char *timestamp,
                          const char *content, int msg_type, int reply_to) {
    char line[TUI_MSG_WIDTH];
    if (reply_to > 0) {
        snprintf(line, sizeof(line), "  [↩ #%d]", reply_to);
        tui_printf("%s", line);
    }
    switch (msg_type) {
        case MSG_TYPE_NORMAL:
            snprintf(line, sizeof(line), "[%s] %s: %s", timestamp, from_nick, content);
            break;
        case MSG_TYPE_SYSTEM:
            snprintf(line, sizeof(line), "--- %s ---", content);
            break;
        case MSG_TYPE_WHISPER:
            snprintf(line, sizeof(line), "[귓속말] %s: %s", from_nick, content);
            break;
        case MSG_TYPE_ME:
            snprintf(line, sizeof(line), "* %s %s", from_nick, content);
            break;
        default:
            snprintf(line, sizeof(line), "[%s] %s: %s", timestamp, from_nick, content);
            break;
    }
    tui_printf("%s", line);
}

/* ─────────────────────────────────────────────────────────
 * packet_parse
 * RecvMsg 스레드에서 호출. buf는 '\n' 없는 단일 줄 문자열.
 * ───────────────────────────────────────────────────────── */
void packet_parse(const char *buf, SOCKET sock) {
    (void)sock;

    char tmp[MAX_BUF_SIZE];
    strncpy(tmp, buf, MAX_BUF_SIZE - 1);
    tmp[MAX_BUF_SIZE - 1] = '\0';

    int tlen = (int)strlen(tmp);
    if (tlen > 0 && tmp[tlen - 1] == '\r')
        tmp[--tlen] = '\0';

    char *type    = strtok(tmp, "|");
    char *payload = strtok(NULL, "");
    if (!type) return;

    /* ══════════════════════════════
     *  인증
     * ══════════════════════════════ */
    if (strcmp(type, LOGIN_RES) == 0) {
        int code = payload ? atoi(payload) : -1;
        g_last_code = code;
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (code == LOGIN_OK) {
            g_state.logged_in = 1;
            printf("[로그인 성공] %s님, 환영합니다!\n", g_state.user_id);
        } else {
            g_state.logged_in = 0;
            static const char *errs[] = {
                "", "아이디를 찾을 수 없습니다.", "비밀번호가 틀렸습니다.",
                "이미 접속 중인 계정입니다."
            };
            printf("[로그인 실패] %s\n",
                   (code >= 1 && code <= 3) ? errs[code] : "알 수 없는 오류");
        }
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, REGISTER_RES) == 0) {
        int code = payload ? atoi(payload) : -1;
        g_last_code = code;
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (code == REGISTER_OK) {
            printf("[회원가입 성공] 이제 로그인하세요.\n");
        } else {
            static const char *errs[] = {
                "알 수 없는 오류", "알 수 없는 오류",
                "이미 사용 중인 아이디입니다.", "오류가 발생했습니다."
            };
            printf("[회원가입 실패] %s\n",
                   (code >= 0 && code <= 3) ? errs[code] : "알 수 없는 오류");
        }
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, LOGOUT_RES) == 0) {
        g_state.logged_in = 0;
        g_state.response_received = 1;
    }

    /* ══════════════════════════════
     *  채팅방
     * ══════════════════════════════ */
    else if (strcmp(type, ROOM_CREATE_RES) == 0) {
        if (!payload) { g_state.response_received = 1; return; }
        char *code_s  = strtok(payload, ":");
        char *rid_s   = strtok(NULL, ":");
        int   code    = code_s ? atoi(code_s) : 0;
        int   room_id = rid_s  ? atoi(rid_s)  : 0;
        g_last_code    = code;
        g_last_room_id = room_id;
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (code == ROOM_CREATE_OK)
            printf("[채팅방 생성 성공] 방 ID: %d\n", room_id);
        else
            printf("[채팅방 생성 실패]\n");
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, ROOM_JOIN_RES) == 0) {
        if (!payload) { g_state.response_received = 1; return; }
        char *code_s = strtok(payload, ":");
        char *rid_s  = strtok(NULL, ":");
        char *rname  = strtok(NULL, "");  /* room_name content-last */
        int   code    = code_s ? atoi(code_s) : 1;
        int   room_id = rid_s  ? atoi(rid_s)  : 0;
        g_last_code = code;
        if (code == ROOM_JOIN_OK) {
            g_state.current_room_id = room_id;
            if (rname)
                strncpy(g_state.current_room_name, rname,
                        sizeof(g_state.current_room_name) - 1);
            WaitForSingleObject(g_console_mutex, INFINITE);
            printf("[채팅방 입장] %s\n", rname ? rname : "");
            fflush(stdout);
            ReleaseMutex(g_console_mutex);
        } else {
            static const char *errs[] = {
                "", "방을 찾을 수 없습니다", "비밀번호가 틀렸습니다", "방이 가득 찼습니다"
            };
            WaitForSingleObject(g_console_mutex, INFINITE);
            printf("[입장 실패] %s\n",
                   (code >= 1 && code <= 3) ? errs[code] : "오류");
            fflush(stdout);
            ReleaseMutex(g_console_mutex);
        }
        g_state.response_received = 1;
    }
    else if (strcmp(type, ROOM_LIST_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload || !*payload) {
            printf("  (참여 가능한 방이 없습니다)\n");
        } else {
            printf("\n  %-5s %-22s %6s %4s %4s  %s\n",
                   "ID", "방이름", "인원", "최대", "비번", "주제");
            printf("  %-5s %-22s %6s %4s %4s  %s\n",
                   "-----", "----------------------",
                   "------", "----", "----", "--------");
            char *sp1;
            char *room_tok = safe_strtok_r(payload, ";", &sp1);
            int cnt = 0;
            while (room_tok) {
                char *sp2;
                char *id_s    = safe_strtok_r(room_tok, ":", &sp2);
                char *name    = safe_strtok_r(NULL,     ":", &sp2);
                char *cur     = safe_strtok_r(NULL,     ":", &sp2);
                char *max_s   = safe_strtok_r(NULL,     ":", &sp2);
                char *has_pw  = safe_strtok_r(NULL,     ":", &sp2);
                safe_strtok_r(NULL, ":", &sp2);          /* is_open 건너뜀 */
                char *topic   = safe_strtok_r(NULL,     "", &sp2);  /* content-last */
                if (id_s && name) {
                    printf("  %-5s %-22s %3s/%s    %s   %s\n",
                           id_s, name,
                           cur   ? cur   : "?",
                           max_s ? max_s : "?",
                           (has_pw && has_pw[0] == '1') ? "Y" : "N",
                           (topic  && *topic)            ? topic : "");
                    cnt++;
                }
                room_tok = safe_strtok_r(NULL, ";", &sp1);
            }
            if (cnt == 0)
                printf("  (참여 가능한 방이 없습니다)\n");
        }
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, ROOM_MSG_RECV) == 0) {
        if (!payload) return;
        /* room_id:from_nick:timestamp:msg_id:reply_to_id:msg_type:content(last) */
        char *room_id_s  = strtok(payload, ":");
        char *from_nick  = strtok(NULL, ":");
        char *timestamp  = strtok(NULL, ":");
        strtok(NULL, ":");                /* msg_id 건너뜀 */
        char *reply_s    = strtok(NULL, ":");
        char *mtype_s    = strtok(NULL, ":");
        char *content    = strtok(NULL, "");  /* content-last */

        if (!room_id_s || !from_nick || !timestamp || !reply_s || !mtype_s || !content)
            return;

        int room_id  = atoi(room_id_s);
        int reply_to = atoi(reply_s);
        int msg_type = atoi(mtype_s);

        if (room_id == g_state.current_room_id) {
            display_chat_message(from_nick, timestamp, content, msg_type, reply_to);
        } else if (!g_state.dnd) {
            tui_printf("[알림] 채팅방 #%d: %s", room_id, content);
        }
    }
    else if (strcmp(type, ROOM_KICKED_NOTIFY) == 0) {
        if (!payload) return;
        strtok(payload, ":");          /* room_id */
        char *by_nick = strtok(NULL, ":");
        g_state.current_room_id      = 0;
        g_state.current_room_name[0] = '\0';
        tui_printf("[강퇴] %s님에 의해 채팅방에서 강퇴되었습니다.",
                   by_nick ? by_nick : "관리자");
    }
    else if (strcmp(type, ROOM_NOTICE) == 0) {
        if (!payload) return;
        strtok(payload, ":");          /* room_id 건너뜀 */
        char *notice = strtok(NULL, "");
        if (notice)
            tui_printf("[공지] %s", notice);
    }
    else if (strcmp(type, ROOM_INVITE_NOTIFY) == 0) {
        if (!payload) return;
        char *rid_s    = strtok(payload, ":");
        char *rname    = strtok(NULL, ":");
        char *inv_nick = strtok(NULL, ":");
        tui_printf("[초대] %s님이 '%s' 방에 초대했습니다. (방 ID: %s)",
                   inv_nick ? inv_nick : "누군가",
                   rname    ? rname    : "",
                   rid_s    ? rid_s    : "?");
    }
    else if (strcmp(type, ROOM_INVITE_RES) == 0) {
        int code = payload ? atoi(payload) : -1;
        g_last_code = code;
        static const char *inv_msgs[] = {
            "초대를 보냈습니다.", "사용자를 찾을 수 없습니다.",
            "이미 방에 있는 사용자입니다.", "방이 가득 찼습니다."
        };
        tui_printf("[초대] %s",
                   (code >= 0 && code <= 3) ? inv_msgs[code] : "오류");
        g_state.response_received = 1;
    }

    /* ══════════════════════════════
     *  메시지 편집/삭제 알림
     * ══════════════════════════════ */
    else if (strcmp(type, MSG_EDITED_NOTIFY) == 0) {
        if (!payload) return;
        char *rid_s   = strtok(payload, ":");
        char *mid_s   = strtok(NULL, ":");
        char *content = strtok(NULL, "");  /* content-last */
        int room_id   = rid_s ? atoi(rid_s) : -1;
        if (room_id == g_state.current_room_id || room_id == 0)
            tui_printf("[수정됨] #%s: %s",
                       mid_s ? mid_s : "?", content ? content : "");
    }
    else if (strcmp(type, MSG_DELETED_NOTIFY) == 0) {
        if (!payload) return;
        char *rid_s = strtok(payload, ":");
        char *mid_s = strtok(NULL, ":");
        int room_id = rid_s ? atoi(rid_s) : -1;
        if (room_id == g_state.current_room_id || room_id == 0)
            tui_printf("[삭제됨] 메시지 #%s가 삭제되었습니다.",
                       mid_s ? mid_s : "?");
    }
    else if (strcmp(type, MSG_SEARCH_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload || !*payload) {
            printf("  (검색 결과 없음)\n");
        } else {
            char tmp2[MAX_BUF_SIZE];
            strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
            tmp2[MAX_BUF_SIZE - 1] = '\0';
            char *sp_cnt;
            char *cnt_s = safe_strtok_r(tmp2, ":", &sp_cnt);
            int   cnt   = cnt_s ? atoi(cnt_s) : 0;
            if (cnt <= 0 || !sp_cnt || !*sp_cnt) {
                printf("  (검색 결과 없음)\n");
            } else {
                printf("\n  메시지 검색 결과 (%d개):\n", cnt);
                char *sp_entry;
                char *entry = safe_strtok_r(sp_cnt, ";", &sp_entry);
                int remaining = cnt;
                while (entry && remaining-- > 0) {
                    char ecpy[MAX_BUF_SIZE];
                    strncpy(ecpy, entry, MAX_BUF_SIZE - 1);
                    ecpy[MAX_BUF_SIZE - 1] = '\0';
                    char *sp2;
                    char *mid_s   = safe_strtok_r(ecpy, ":", &sp2);
                    char *nick    = safe_strtok_r(NULL,  ":", &sp2);
                    char *ts      = safe_strtok_r(NULL,  ":", &sp2);
                    char *content = safe_strtok_r(NULL,  "", &sp2);
                    if (mid_s && nick && content)
                        printf("  [#%s][%s] %s: %s\n",
                               mid_s, ts ? ts : "?", nick, content);
                    entry = safe_strtok_r(NULL, ";", &sp_entry);
                }
            }
        }
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }

    /* ══════════════════════════════
     *  DM
     * ══════════════════════════════ */
    else if (strcmp(type, DM_RECV) == 0) {
        if (!payload) return;
        char *from_id   = strtok(payload, ":");
        char *from_nick = strtok(NULL, ":");
        char *timestamp = strtok(NULL, ":");
        strtok(NULL, ":");    /* msg_id 건너뜀 */
        char *content   = strtok(NULL, "");
        if (!from_id || !from_nick || !content) return;

        /* DM 대화 중인 상대에게서 온 메시지면 인라인 표시 */
        if (g_state.current_dm_partner[0] != '\0' &&
            strcmp(from_id, g_state.current_dm_partner) == 0) {
            display_chat_message(from_nick, timestamp ? timestamp : "",
                                 content, MSG_TYPE_NORMAL, 0);
        } else if (!g_state.dnd) {
            tui_printf("[DM] %s: %s", from_nick, content);
        }
    }
    else if (strcmp(type, DM_LIST_RES) == 0) {
        g_dm_count = 0;
        if (!payload || !*payload) {
            g_state.response_received = 1;
            return;
        }
        char tmp2[MAX_BUF_SIZE];
        strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
        tmp2[MAX_BUF_SIZE - 1] = '\0';

        char *sp_cnt;
        char *cnt_s = safe_strtok_r(tmp2, ":", &sp_cnt);
        int total   = cnt_s ? atoi(cnt_s) : 0;
        if (total <= 0 || !sp_cnt || *sp_cnt == '\0') {
            g_state.response_received = 1;
            return;
        }

        char *sp_entry;
        char *entry = safe_strtok_r(sp_cnt, ";", &sp_entry);
        while (entry && g_dm_count < MAX_DM_LIST) {
            char ecpy[512];
            strncpy(ecpy, entry, 511);
            ecpy[511] = '\0';
            char *sp2;
            char *pid_s    = safe_strtok_r(ecpy, ":", &sp2);
            char *pnick_s  = safe_strtok_r(NULL,  ":", &sp2);
            safe_strtok_r(NULL, ":", &sp2);     /* timestamp 건너뜀 */
            char *unread_s = safe_strtok_r(NULL,  ":", &sp2);
            char *lmsg_s   = safe_strtok_r(NULL,  "", &sp2);  /* content-last */

            if (pid_s && pnick_s) {
                DmPartnerEntry *de = &g_dm_list[g_dm_count++];
                strncpy(de->partner_id,   pid_s,   20); de->partner_id[20]   = '\0';
                strncpy(de->partner_nick, pnick_s, 20); de->partner_nick[20] = '\0';
                de->unread = unread_s ? atoi(unread_s) : 0;
                if (lmsg_s) { strncpy(de->last_msg, lmsg_s, 100); de->last_msg[100] = '\0'; }
                else          de->last_msg[0] = '\0';
            }
            entry = safe_strtok_r(NULL, ";", &sp_entry);
        }
        g_state.response_received = 1;
    }
    else if (strcmp(type, DM_HISTORY_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload || !*payload) {
            tui_puts("  (이전 메시지가 없습니다)");
        } else {
            char tmp2[MAX_BUF_SIZE];
            strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
            tmp2[MAX_BUF_SIZE - 1] = '\0';

            char *sp_cnt;
            char *cnt_s = safe_strtok_r(tmp2, ":", &sp_cnt);
            int cnt     = cnt_s ? atoi(cnt_s) : 0;

            if (cnt > 0 && sp_cnt && *sp_cnt) {
                char *sp_entry;
                char *entry = safe_strtok_r(sp_cnt, ";", &sp_entry);
                while (entry && cnt-- > 0) {
                    char ecpy[512];
                    strncpy(ecpy, entry, 511);
                    ecpy[511] = '\0';
                    char *sp2;
                    safe_strtok_r(ecpy, ":", &sp2);      /* msg_id 건너뜀 */
                    char *from_id_s = safe_strtok_r(NULL, ":", &sp2);
                    char *ts        = safe_strtok_r(NULL, ":", &sp2);
                    safe_strtok_r(NULL, ":", &sp2);      /* read 건너뜀 */
                    char *content   = safe_strtok_r(NULL, "", &sp2);  /* content-last */

                    if (from_id_s && content) {
                        int is_me = (strcmp(from_id_s, g_state.user_id) == 0);
                        const char *nick = is_me ? g_state.nickname
                                                 : g_state.current_dm_partner_nick;
                        char hline[TUI_MSG_WIDTH];
                        snprintf(hline, sizeof(hline), "[%s] %s: %s",
                                 ts ? ts : "?",
                                 (nick && nick[0]) ? nick : from_id_s,
                                 content);
                        tui_puts(hline);
                    }
                    entry = safe_strtok_r(NULL, ";", &sp_entry);
                }
            } else {
                tui_puts("  (이전 메시지가 없습니다)");
            }
        }
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, DM_READ_NOTIFY) == 0) {
        /* DM 읽음 처리 — 다음 DM_LIST_REQ 갱신 시 미읽음 수에 반영됨 */
        (void)payload;
    }

    else if (strcmp(type, ROOM_HISTORY_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload || !*payload) {
            printf("  (이전 메시지가 없습니다)\n");
        } else {
            char tmp2[MAX_BUF_SIZE];
            strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
            tmp2[MAX_BUF_SIZE - 1] = '\0';

            char *sp_cnt;
            char *cnt_s = safe_strtok_r(tmp2, ":", &sp_cnt);
            int   cnt   = cnt_s ? atoi(cnt_s) : 0;

            if (cnt > 0 && sp_cnt && *sp_cnt) {
                char *sp_entry;
                char *entry = safe_strtok_r(sp_cnt, ";", &sp_entry);
                while (entry && cnt-- > 0) {
                    char ecpy[MAX_BUF_SIZE];
                    strncpy(ecpy, entry, MAX_BUF_SIZE - 1);
                    ecpy[MAX_BUF_SIZE - 1] = '\0';
                    char *sp2;
                    safe_strtok_r(ecpy, ":", &sp2);            /* msg_id */
                    char *from_nick = safe_strtok_r(NULL, ":", &sp2);
                    char *ts        = safe_strtok_r(NULL, ":", &sp2);
                    safe_strtok_r(NULL, ":", &sp2);            /* reply_to */
                    char *mtype_s   = safe_strtok_r(NULL, ":", &sp2);
                    char *content   = safe_strtok_r(NULL, "", &sp2); /* content-last */

                    if (content) {
                        int mtype = mtype_s ? atoi(mtype_s) : 0;
                        char hline[TUI_MSG_WIDTH];
                        if (mtype == 1)
                            snprintf(hline, sizeof(hline), "  --- %s ---", content);
                        else
                            snprintf(hline, sizeof(hline), "[%s] %s: %s",
                                     ts ? ts : "?",
                                     from_nick ? from_nick : "?",
                                     content);
                        tui_puts(hline);
                    }
                    entry = safe_strtok_r(NULL, ";", &sp_entry);
                }
            } else {
                tui_puts("  (이전 메시지가 없습니다)");
            }
        }
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }

    /* ══════════════════════════════
     *  친구
     * ══════════════════════════════ */
    else if (strcmp(type, FRIEND_LIST_RES) == 0) {
        g_friend_count = 0;
        if (!payload || !*payload) {
            g_state.response_received = 1;
            return;
        }
        char tmp2[MAX_BUF_SIZE];
        strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
        tmp2[MAX_BUF_SIZE - 1] = '\0';

        char *sp_cnt;
        char *cnt_s = safe_strtok_r(tmp2, ":", &sp_cnt);
        int total   = cnt_s ? atoi(cnt_s) : 0;
        if (total <= 0 || !sp_cnt || *sp_cnt == '\0') {
            g_state.response_received = 1;
            return;
        }

        char *sp_entry;
        char *entry = safe_strtok_r(sp_cnt, ";", &sp_entry);
        while (entry && g_friend_count < MAX_FRIEND_LIST) {
            char ecpy[512];
            strncpy(ecpy, entry, 511);
            ecpy[511] = '\0';
            char *sp2;
            char *id_s   = safe_strtok_r(ecpy, ":", &sp2);
            char *nick_s = safe_strtok_r(NULL,  ":", &sp2);
            char *stat_s = safe_strtok_r(NULL,  ":", &sp2);
            char *smsg_s = safe_strtok_r(NULL,  "", &sp2);  /* content-last */

            if (id_s && nick_s) {
                FriendEntry *fe = &g_friend_list[g_friend_count++];
                strncpy(fe->id,   id_s,   20); fe->id[20]   = '\0';
                strncpy(fe->nick, nick_s, 20); fe->nick[20] = '\0';
                fe->status = stat_s ? atoi(stat_s) : 0;
                if (smsg_s) { strncpy(fe->status_msg, smsg_s, 100); fe->status_msg[100] = '\0'; }
                else          fe->status_msg[0] = '\0';
            }
            entry = safe_strtok_r(NULL, ";", &sp_entry);
        }
        g_state.response_received = 1;
    }
    else if (strcmp(type, FRIEND_ADD_RES) == 0) {
        int code = payload ? atoi(payload) : -1;
        g_last_code = code;
        WaitForSingleObject(g_console_mutex, INFINITE);
        static const char *add_msgs[] = {
            "친구 요청을 보냈습니다.", "사용자를 찾을 수 없습니다.",
            "차단된 사용자입니다.", "이미 친구입니다."
        };
        printf("[친구 추가] %s\n",
               (code >= 0 && code <= 3) ? add_msgs[code] : "오류");
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, FRIEND_REQUEST_NOTIFY) == 0) {
        if (!payload) return;
        char *from_id   = strtok(payload, ":");
        char *from_nick = strtok(NULL, ":");
        if (!from_id || !from_nick) return;
        tui_printf("[친구 요청] %s(%s)님이 친구 요청을 보냈습니다.",
                   from_nick, from_id);
    }
    else if (strcmp(type, FRIEND_STATUS_CHANGE) == 0) {
        if (!payload) return;
        char *id_s      = strtok(payload, ":");
        char *nick      = strtok(NULL, ":");
        char *status_s  = strtok(NULL, ":");
        if (!id_s || !nick || !status_s) return;
        int status = atoi(status_s);
        if (status < 0 || status > 3) status = 0;
        static const char *labels[] = {"오프라인", "온라인", "바쁨", "오프라인"};
        tui_printf("[알림] %s(%s) → %s", nick, id_s, labels[status]);
    }
    else if (strcmp(type, FRIEND_ACCEPT_NOTIFY) == 0) {
        if (!payload) return;
        char *id_s = strtok(payload, ":");
        char *nick = strtok(NULL, ":");
        if (!id_s || !nick) return;
        tui_printf("[친구] %s(%s)님이 친구 요청을 수락했습니다.", nick, id_s);
    }

    /* ══════════════════════════════
     *  마이페이지
     * ══════════════════════════════ */
    else if (strcmp(type, MYPAGE_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload) {
            printf("[마이페이지] 데이터를 가져올 수 없습니다.\n");
        } else {
            char tmp2[MAX_BUF_SIZE];
            strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
            tmp2[MAX_BUF_SIZE - 1] = '\0';
            char *sp2;
            char *id_s         = safe_strtok_r(tmp2, ":", &sp2);
            char *nick_s       = safe_strtok_r(NULL,  ":", &sp2);
            char *created_s    = safe_strtok_r(NULL,  ":", &sp2);
            char *last_seen_s  = safe_strtok_r(NULL,  ":", &sp2);
            char *msg_cnt_s    = safe_strtok_r(NULL,  ":", &sp2);
            char *room_cnt_s   = safe_strtok_r(NULL,  ":", &sp2);
            char *friend_cnt_s = safe_strtok_r(NULL,  ":", &sp2);
            char *status_msg   = safe_strtok_r(NULL,  "", &sp2);  /* content-last */

            printf("\n  ─────────────────────────────\n");
            printf("  아이디    : %s\n",   id_s        ? id_s        : "");
            printf("  닉네임    : %s\n",   nick_s      ? nick_s      : "");
            printf("  상태메시지: %s\n",   (status_msg && *status_msg) ? status_msg : "(없음)");
            printf("  가입일    : %s\n",   created_s   ? created_s   : "");
            printf("  마지막접속: %s\n",   last_seen_s ? last_seen_s : "");
            printf("  메시지 수 : %s\n",   msg_cnt_s   ? msg_cnt_s   : "0");
            printf("  방 수     : %s\n",   room_cnt_s  ? room_cnt_s  : "0");
            printf("  친구 수   : %s\n",   friend_cnt_s ? friend_cnt_s : "0");
            printf("  ─────────────────────────────\n");

            /* 닉네임이 서버 측에서 변경된 경우 반영 */
            if (nick_s && *nick_s) {
                strncpy(g_state.nickname, nick_s, 20);
                g_state.nickname[20] = '\0';
            }
        }
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, PROFILE_UPDATE_RES) == 0) {
        int code = payload ? atoi(payload) : -1;
        g_last_code = code;
        tui_printf(code == 0 ? "[프로필 수정 성공] 변경되었습니다."
                             : "[프로필 수정 실패] 이미 사용 중인 닉네임입니다.");
        g_state.response_received = 1;
    }
    else if (strcmp(type, PASS_CHANGE_RES) == 0) {
        int code = payload ? atoi(payload) : -1;
        g_last_code = code;
        tui_printf(code == 0 ? "[비밀번호 변경 성공] 변경되었습니다."
                             : "[비밀번호 변경 실패] 현재 비밀번호가 틀렸습니다.");
        g_state.response_received = 1;
    }

    /* ══════════════════════════════
     *  설정
     * ══════════════════════════════ */
    else if (strcmp(type, SETTINGS_RES) == 0) {
        if (payload && *payload) {
            char tmp2[256];
            strncpy(tmp2, payload, 255);
            tmp2[255] = '\0';
            char *sp2;
            char *mc    = safe_strtok_r(tmp2, ":", &sp2);
            char *nc    = safe_strtok_r(NULL,  ":", &sp2);
            char *th    = safe_strtok_r(NULL,  ":", &sp2);
            char *tf    = safe_strtok_r(NULL,  ":", &sp2);
            char *dnd_s = safe_strtok_r(NULL,  ":", &sp2);
            if (mc)    { strncpy(g_state.msg_color,  mc, 15); g_state.msg_color[15]  = '\0'; }
            if (nc)    { strncpy(g_state.nick_color, nc, 15); g_state.nick_color[15] = '\0'; }
            if (th)    { strncpy(g_state.theme,      th, 10); g_state.theme[10]      = '\0'; }
            if (tf)      g_state.ts_format = atoi(tf);
            if (dnd_s)   g_state.dnd = atoi(dnd_s);
        }
        g_state.response_received = 1;
    }
    else if (strcmp(type, SETTINGS_UPDATE_RES) == 0) {
        int code = payload ? atoi(payload) : -1;
        g_last_code = code;
        tui_printf(code == 0 ? "[설정 저장 성공]" : "[설정 저장 실패]");
        g_state.response_received = 1;
    }

    /* ══════════════════════════════
     *  타이핑 인디케이터
     * ══════════════════════════════ */
    else if (strcmp(type, TYPING_NOTIFY) == 0) {
        if (!payload) return;
        char *rid_s     = strtok(payload, ":");
        char *nick      = strtok(NULL, ":");
        char *typing_s  = strtok(NULL, ":");
        if (!rid_s || !nick || !typing_s) return;
        int room_id   = atoi(rid_s);
        int is_typing = atoi(typing_s);
        if (room_id == g_state.current_room_id && is_typing)
            tui_printf("[%s 님이 입력 중...]", nick);
    }

    /* ══════════════════════════════
     *  범용 알림
     * ══════════════════════════════ */
    else if (strcmp(type, NOTIFY) == 0) {
        if (!payload) return;
        strtok(payload, ":");          /* sub_type 건너뜀 */
        char *content = strtok(NULL, "");
        tui_printf("[알림] %s", content ? content : "");
    }

    /* ══════════════════════════════
     *  P2 방 관련 응답
     * ══════════════════════════════ */
    else if (strcmp(type, ROOM_DELETED_NOTIFY) == 0) {
        if (!payload) return;
        int room_id = atoi(payload);
        g_state.current_room_id      = 0;
        g_state.current_room_name[0] = '\0';
        tui_printf("[알림] 채팅방 #%d가 삭제되었습니다.", room_id);
    }
    else if (strcmp(type, ROOM_MEMBERS_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload || !*payload) {
            tui_puts("  (멤버 없음)");
        } else {
            char tmp2[MAX_BUF_SIZE];
            strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
            tmp2[MAX_BUF_SIZE - 1] = '\0';
            char *sp_cnt;
            char *cnt_s = safe_strtok_r(tmp2, ":", &sp_cnt);
            int   cnt   = cnt_s ? atoi(cnt_s) : 0;
            char hline[TUI_MSG_WIDTH];
            snprintf(hline, sizeof(hline), "  멤버 목록 (%d명):", cnt);
            tui_puts(hline);
            if (cnt > 0 && sp_cnt && *sp_cnt) {
                char *sp_entry;
                char *entry = safe_strtok_r(sp_cnt, ";", &sp_entry);
                static const char *ostatus[] = {"오프", "온라인", "바쁨", "오프"};
                while (entry) {
                    char ecpy[256];
                    strncpy(ecpy, entry, 255); ecpy[255] = '\0';
                    char *sp2;
                    char *uid    = safe_strtok_r(ecpy, ":", &sp2);
                    char *nick   = safe_strtok_r(NULL,  ":", &sp2);
                    char *adm_s  = safe_strtok_r(NULL,  ":", &sp2);
                    char *onl_s  = safe_strtok_r(NULL,  "", &sp2);
                    if (uid && nick) {
                        int ost = (onl_s && atoi(onl_s) >= 0 && atoi(onl_s) <= 3)
                                  ? atoi(onl_s) : 0;
                        snprintf(hline, sizeof(hline), "    %s (%s) %s [%s]",
                                 nick, uid,
                                 (adm_s && adm_s[0] == '1') ? "[관리자]" : "",
                                 ostatus[ost]);
                        tui_puts(hline);
                    }
                    entry = safe_strtok_r(NULL, ";", &sp_entry);
                }
            }
        }
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, ROOM_SEARCH_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload || !*payload) {
            printf("  (검색 결과 없음)\n");
        } else {
            char tmp2[MAX_BUF_SIZE];
            strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
            tmp2[MAX_BUF_SIZE - 1] = '\0';
            char *sp_cnt;
            char *cnt_s = safe_strtok_r(tmp2, ":", &sp_cnt);
            int   cnt   = cnt_s ? atoi(cnt_s) : 0;
            if (cnt <= 0 || !sp_cnt || !*sp_cnt) {
                printf("  (검색 결과 없음)\n");
            } else {
                printf("\n  %-5s %-22s %6s %4s %4s  %s\n",
                       "ID", "방이름", "인원", "최대", "비번", "주제");
                printf("  %-5s %-22s %6s %4s %4s  %s\n",
                       "-----", "----------------------",
                       "------", "----", "----", "--------");
                char *sp_entry;
                char *entry = safe_strtok_r(sp_cnt, ";", &sp_entry);
                while (entry) {
                    char ecpy[512];
                    strncpy(ecpy, entry, 511); ecpy[511] = '\0';
                    char *sp2;
                    char *id_s   = safe_strtok_r(ecpy, ":", &sp2);
                    char *name   = safe_strtok_r(NULL,  ":", &sp2);
                    char *cur    = safe_strtok_r(NULL,  ":", &sp2);
                    char *max_s  = safe_strtok_r(NULL,  ":", &sp2);
                    char *has_pw = safe_strtok_r(NULL,  ":", &sp2);
                    safe_strtok_r(NULL, ":", &sp2);
                    char *topic  = safe_strtok_r(NULL,  "", &sp2);
                    if (id_s && name) {
                        printf("  %-5s %-22s %3s/%s    %s   %s\n",
                               id_s, name,
                               cur   ? cur   : "?",
                               max_s ? max_s : "?",
                               (has_pw && has_pw[0] == '1') ? "Y" : "N",
                               (topic && *topic) ? topic : "");
                    }
                    entry = safe_strtok_r(NULL, ";", &sp_entry);
                }
            }
        }
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, ROOM_MUTE_TOGGLE_RES) == 0) {
        if (!payload) { g_state.response_received = 1; return; }
        strtok(payload, ":");
        char *mute_s = strtok(NULL, "");
        int   muted  = mute_s ? atoi(mute_s) : 0;
        tui_printf("[알림] 방 알림 %s", muted ? "끄기 완료" : "켜기 완료");
        g_state.response_received = 1;
    }
    else if (strcmp(type, ROOM_SET_OPEN_NICK_RES) == 0) {
        int code = payload ? atoi(payload) : -1;
        g_last_code = code;
        tui_printf(code == 0 ? "[오픈채팅 닉네임 변경 성공]"
                             : "[오픈채팅 닉네임 변경 실패]");
        g_state.response_received = 1;
    }
    else if (strcmp(type, WHISPER_RECV) == 0) {
        if (!payload) return;
        char *rid_s     = strtok(payload, ":");
        char *from_id   = strtok(NULL,    ":");
        char *from_nick = strtok(NULL,    ":");
        char *content   = strtok(NULL,    "");
        if (!rid_s || !from_id || !from_nick || !content) return;
        int room_id = atoi(rid_s);
        if (room_id == g_state.current_room_id)
            tui_printf("[귓속말] %s: %s", from_nick, content);
    }
    else if (strcmp(type, MSG_PIN_NOTIFY) == 0) {
        if (!payload) return;
        char *rid_s = strtok(payload, ":");
        char *mid_s = strtok(NULL, "");
        int room_id = rid_s ? atoi(rid_s) : 0;
        if (room_id == g_state.current_room_id) {
            if (mid_s && atoi(mid_s) > 0)
                tui_printf("[핀 메시지] #%s", mid_s);
            else
                tui_printf("[핀 메시지 해제됨]");
        }
    }

    /* ══════════════════════════════
     *  사용자 검색 / 내 방 목록
     * ══════════════════════════════ */
    else if (strcmp(type, USER_SEARCH_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload || !*payload) {
            printf("  (검색 결과 없음)\n");
        } else {
            char tmp2[MAX_BUF_SIZE];
            strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
            tmp2[MAX_BUF_SIZE - 1] = '\0';
            char *sp_cnt;
            char *cnt_s = safe_strtok_r(tmp2, ":", &sp_cnt);
            int   cnt   = cnt_s ? atoi(cnt_s) : 0;
            if (cnt <= 0 || !sp_cnt || !*sp_cnt) {
                printf("  (검색 결과 없음)\n");
            } else {
                printf("\n  유저 검색 결과 (%d명):\n", cnt);
                char *sp_entry;
                char *entry = safe_strtok_r(sp_cnt, ";", &sp_entry);
                int i = 1;
                while (entry) {
                    char ecpy[128];
                    strncpy(ecpy, entry, 127); ecpy[127] = '\0';
                    char *sp2;
                    char *uid  = safe_strtok_r(ecpy, ":", &sp2);
                    char *nick = safe_strtok_r(NULL,  "", &sp2);
                    if (uid && nick)
                        printf("    %d. %s (%s)\n", i++, nick, uid);
                    entry = safe_strtok_r(NULL, ";", &sp_entry);
                }
            }
        }
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, MY_ROOMS_RES) == 0) {
        WaitForSingleObject(g_console_mutex, INFINITE);
        if (!payload || !*payload) {
            printf("  (참여 중인 방 없음)\n");
        } else {
            char tmp2[MAX_BUF_SIZE];
            strncpy(tmp2, payload, MAX_BUF_SIZE - 1);
            tmp2[MAX_BUF_SIZE - 1] = '\0';
            printf("\n  참여 중인 방 목록:\n");
            char *sp1;
            char *entry = safe_strtok_r(tmp2, ";", &sp1);
            int   i     = 1;
            while (entry) {
                char ecpy[128];
                strncpy(ecpy, entry, 127); ecpy[127] = '\0';
                char *sp2;
                char *rid_s = safe_strtok_r(ecpy, ":", &sp2);
                char *rname = safe_strtok_r(NULL,  "", &sp2);
                if (rid_s && rname)
                    printf("    %d. %s (ID: %s)\n", i++, rname, rid_s);
                entry = safe_strtok_r(NULL, ";", &sp1);
            }
        }
        fflush(stdout);
        ReleaseMutex(g_console_mutex);
        g_state.response_received = 1;
    }
    else if (strcmp(type, DM_READ_NOTIFY) == 0) {
        /* 읽음 알림 — 조용히 처리 */
        (void)payload;
    }

    /* ══════════════════════════════
     *  Keep-Alive
     * ══════════════════════════════ */
    else if (strcmp(type, PONG) == 0) {
        g_state.last_pong = time(NULL);
    }
}
