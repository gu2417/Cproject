#pragma once

#include "../common/types.h"

/* =========================================================
 * users.txt  포맷:
 * id//pw_hash//nickname//status_msg//online_status//is_admin//last_seen//created_at
 * ========================================================= */
/* 사용자 파일을 읽어서 메모리에 올린다. */
int  load_users(const char *path);
/* 메모리에 있는 사용자 목록을 파일에 다시 저장한다. */
void save_users(const char *path);
/* 사용자 한 명을 파일 끝에 추가한다. */
int  append_user(const char *path, const UserRecord *u);

/* =========================================================
 * rooms.txt  포맷:
 * id//name//topic//pw_hash//max_users//owner_id//notice//is_open//pinned_msg_id//created_at
 * ========================================================= */
/* 채팅방 파일을 읽어서 메모리에 올린다. */
int  load_rooms(const char *path);
/* 메모리에 있는 채팅방 목록을 파일에 다시 저장한다. */
void save_rooms(const char *path);
/* 채팅방 한 개를 파일 끝에 추가한다. */
int  append_room(const char *path, const RoomRecord *r);

/* =========================================================
 * room_members.txt  포맷:
 * room_id//user_id//open_nick//is_admin//is_muted//joined_at
 * ========================================================= */
/* 방 멤버 파일을 읽어서 메모리에 올린다. */
int  load_room_members(const char *path);
/* 메모리에 있는 방 멤버 목록을 파일에 다시 저장한다. */
void save_room_members(const char *path);
/* 방 멤버 한 명을 파일 끝에 추가한다. */
int  append_room_member(const char *path, const RoomMemberRecord *m);

/* =========================================================
 * messages.txt  포맷 (content-last):
 * id//room_id//from_id//to_id//reply_to//msg_type//is_deleted//created_at//edited_at//content
 * ========================================================= */
/* 메시지 파일을 읽어서 메모리에 올린다. */
int  load_messages(const char *path);
/* 메모리에 있는 메시지 목록을 파일에 다시 저장한다. */
void save_messages(const char *path);
/* 메시지 한 개를 파일 끝에 추가한다. */
int  append_message(const char *path, const MessageRecord *m);

/* =========================================================
 * friends.txt  포맷:
 * id//user_id//friend_id//status//status_before_block//created_at
 * ========================================================= */
/* 친구 파일을 읽어서 메모리에 올린다. */
int  load_friends(const char *path);
/* 메모리에 있는 친구 목록을 파일에 다시 저장한다. */
void save_friends(const char *path);
/* 친구 관계 한 개를 파일 끝에 추가한다. */
int  append_friend(const char *path, const FriendRecord *f);

/* =========================================================
 * dm_reads.txt  포맷:
 * msg_id//reader_id//read_at
 * ========================================================= */
/* DM 읽음 기록 파일을 읽어서 메모리에 올린다. */
int  load_dm_reads(const char *path);
/* 메모리에 있는 DM 읽음 기록을 파일에 다시 저장한다. */
void save_dm_reads(const char *path);
/* DM 읽음 기록 한 개를 파일 끝에 추가한다. */
int  append_dm_read(const char *path, const DmReadRecord *r);

/* =========================================================
 * room_invites.txt  포맷:
 * id//room_id//inviter_id//invitee_id//status//created_at
 * ========================================================= */
/* 방 초대 파일을 읽어서 메모리에 올린다. */
int  load_room_invites(const char *path);
/* 메모리에 있는 방 초대 목록을 파일에 다시 저장한다. */
void save_room_invites(const char *path);
/* 방 초대 기록 한 개를 파일 끝에 추가한다. */
int  append_room_invite(const char *path, const RoomInviteRecord *r);

/* =========================================================
 * user_settings.txt  포맷:
 * user_id//msg_color//nick_color//theme//ts_format//dnd//welcome_shown
 * ========================================================= */
/* 사용자 설정 파일을 읽어서 메모리에 올린다. */
int  load_user_settings(const char *path);
/* 메모리에 있는 사용자 설정을 파일에 다시 저장한다. */
void save_user_settings(const char *path);
/* 사용자 설정이 있으면 바꾸고, 없으면 새로 넣는다. */
int  upsert_user_settings(const char *path, const UserSettingsRecord *s);

/* =========================================================
 * room_reads.txt  포맷:
 * room_id//user_id//last_read_msg_id//read_at
 * ========================================================= */
/* 방 읽음 기록 파일을 읽어서 메모리에 올린다. */
int  load_room_reads(const char *path);
/* 메모리에 있는 방 읽음 기록을 파일에 다시 저장한다. */
void save_room_reads(const char *path);
/* 특정 사용자의 방 읽음 위치를 갱신한다. */
int  update_room_read(const char *path, int room_id,
                      const char *user_id, int msg_id,
                      const char *read_at);
