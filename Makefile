CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wno-unused-parameter
SRVSRC  = $(wildcard chat_program/src/server/*.c) chat_program/src/common/utils.c
CLISRC  = $(wildcard chat_program/src/client/*.c) chat_program/src/common/utils.c
SRVOBJ  = $(SRVSRC:.c=.o)
CLIOBJ  = $(CLISRC:.c=.o)
SRVLIBS = -lws2_32 -ladvapi32
CLILIBS = -lws2_32

.PHONY: all server client init clean

all: server client

server: $(SRVSRC)
	$(CC) $(CFLAGS) -o server.exe $(SRVSRC) $(SRVLIBS)

client: $(CLISRC)
	$(CC) $(CFLAGS) -o client.exe $(CLISRC) $(CLILIBS)

init:
	@if not exist data mkdir data
	@type nul > data\users.txt
	@type nul > data\rooms.txt
	@type nul > data\messages.txt
	@type nul > data\friends.txt
	@type nul > data\room_members.txt
	@type nul > data\dm_reads.txt
	@type nul > data\room_invites.txt
	@type nul > data\user_settings.txt
	@type nul > data\room_reads.txt
	@echo data/ initialized (9 files).

clean:
	@del /Q server.exe client.exe 2>nul || true
	@del /Q chat_program\src\server\*.o 2>nul || true
	@del /Q chat_program\src\client\*.o 2>nul || true
	@del /Q chat_program\src\common\*.o 2>nul || true
