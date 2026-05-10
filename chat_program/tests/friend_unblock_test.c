#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include "../src/common/protocol.h"
#include "../src/common/types.h"
#include "../src/server/broadcast.h"
#include "../src/server/file_io.h"
#include "../src/server/friend.h"
#include "../src/server/router.h"
#include "../src/server/user_store.h"
#include "../src/server/globals.h"

void send_packet(SOCKET sock, const char *fmt, ...) { (void)sock; (void)fmt; }
void send_to_user(const char *user_id, const char *msg) { (void)user_id; (void)msg; }
void register_handler(const char *type, PacketHandler fn) { (void)type; (void)fn; }
int append_friend(const char *path, const FriendRecord *f) { (void)path; (void)f; return 0; }
void save_friends(const char *path) { (void)path; }
void get_nickname(const char *user_id, char out_nick[21]) {
    strncpy(out_nick, user_id, 20);
    out_nick[20] = '\0';
}

static int expect_int(const char *name, int actual, int expected) {
    if (actual != expected) {
        printf("FAIL %s: expected %d, got %d\n", name, expected, actual);
        return 0;
    }
    return 1;
}

static void reset_friends(void) {
    memset(g_friends, 0, sizeof(g_friends));
    g_friend_count = 0;
    g_next_friend_id = 1;
}

static int test_restore_accepted_friend(void) {
    reset_friends();
    g_friends[0].id = g_next_friend_id++;
    strcpy(g_friends[0].user_id, "alice");
    strcpy(g_friends[0].friend_id, "bob");
    g_friends[0].status = FRIEND_ACCEPTED;
    g_friends[0].status_before_block = -1;
    strcpy(g_friends[0].created_at, "2026-05-09 00:00:00");
    g_friend_count = 1;

    if (!expect_int("block accepted", friend_block_user("alice", "bob"), 1)) return 0;
    if (!expect_int("blocked status", g_friends[0].status, FRIEND_BLOCKED_S)) return 0;
    if (!expect_int("saved previous status", g_friends[0].status_before_block, FRIEND_ACCEPTED)) return 0;
    if (!expect_int("is blocked", is_blocked_by("alice", "bob"), 1)) return 0;

    if (!expect_int("unblock accepted", friend_unblock_user("alice", "bob"), 1)) return 0;
    if (!expect_int("restored status", g_friends[0].status, FRIEND_ACCEPTED)) return 0;
    if (!expect_int("cleared previous status", g_friends[0].status_before_block, -1)) return 0;
    return expect_int("not blocked", is_blocked_by("alice", "bob"), 0);
}

static int test_remove_block_only_record(void) {
    reset_friends();

    if (!expect_int("block unknown user", friend_block_user("alice", "charlie"), 1)) return 0;
    if (!expect_int("created one record", g_friend_count, 1)) return 0;
    if (!expect_int("block only previous status", g_friends[0].status_before_block, -1)) return 0;

    if (!expect_int("unblock unknown user", friend_unblock_user("alice", "charlie"), 1)) return 0;
    return expect_int("removed block only record", g_friend_count, 0);
}

int main(void) {
    int ok = 1;
    ok = test_restore_accepted_friend() && ok;
    ok = test_remove_block_only_record() && ok;
    printf("%s friend_unblock_test\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
