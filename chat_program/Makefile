CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wno-unused-parameter \
          -Isrc/server -Isrc/client -Isrc/common

SERVER_SRCS = \
    src/server/main.c \
    src/server/globals.c \
    src/server/client_handler.c \
    src/server/router.c \
    src/server/file_io.c \
    src/server/auth.c \
    src/server/user_store.c \
    src/server/friend.c \
    src/server/room.c \
    src/server/dm.c \
    src/server/message.c \
    src/server/broadcast.c \
    src/common/utils.c

CLIENT_SRCS = \
    src/client/main.c \
    src/client/net.c \
    src/client/packet.c \
    src/client/state.c \
    src/client/menu_initial.c \
    src/client/menu_main.c \
    src/client/menu_chat.c \
    src/client/menu_friend.c \
    src/client/menu_dm.c \
    src/client/menu_mypage.c \
    src/client/menu_settings.c \
    src/client/chat_tui.c \
    src/common/utils.c

SERVER_LIBS = -lws2_32 -ladvapi32
CLIENT_LIBS = -lws2_32

all: server.exe client.exe

server.exe: $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(SERVER_LIBS)

client.exe: $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIENT_LIBS)

data:
	mkdir -p data
	touch data/users.txt data/rooms.txt data/messages.txt \
	      data/friends.txt data/room_members.txt data/dm_reads.txt \
	      data/room_invites.txt data/user_settings.txt data/room_reads.txt

clean:
	rm -f server.exe client.exe

.PHONY: all clean data
