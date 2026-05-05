# Makefile for chat_program

UNAME := $(shell uname -s 2>/dev/null || echo Windows)

CC        := gcc
CSTD      := -std=c11 -Wall -Wextra -Wpedantic
OPT       := -O2
INCLUDES  := -Isrc/common

# GTK4 flags (auto-detected via pkg-config)
GTK4_CFLAGS := $(shell pkg-config --cflags gtk4 2>/dev/null || echo "")
GTK4_LIBS   := $(shell pkg-config --libs gtk4 2>/dev/null || echo "")

# Windows GTK4 fallback paths
ifeq ($(OS),Windows_NT)
    ifeq ($(GTK4_CFLAGS),)
        GTK4_CFLAGS := -IC:/msys64/mingw64/include/gtk-4.0 -IC:/msys64/mingw64/include/glib-2.0
    endif
    ifeq ($(GTK4_LIBS),)
        GTK4_LIBS := -LC:/msys64/mingw64/lib -lgtk-4
    endif
endif

# MySQL flags
MYSQL_CFLAGS := -IC:/msys64/mingw64/include/include
MYSQL_LIBS   := -LC:/msys64/mingw64/lib/lib -lmysql

# Platform-specific flags
ifeq ($(UNAME),Linux)
    LDFLAGS_SRV := -lpthread $(MYSQL_LIBS)
    LDFLAGS_CLI := -lpthread $(GTK4_LIBS)
    CFLAGS_SRV  := $(MYSQL_CFLAGS)
    CFLAGS_CLI  := $(GTK4_CFLAGS)
endif
ifneq (,$(findstring MINGW,$(UNAME))$(findstring Windows,$(UNAME)))
    LDFLAGS_SRV := -lpthread -lws2_32 $(MYSQL_LIBS)
    LDFLAGS_CLI := -lpthread -lws2_32 $(GTK4_LIBS)
    CFLAGS_SRV  := $(MYSQL_CFLAGS)
    CFLAGS_CLI  := $(GTK4_CFLAGS)
    EXE         := .exe
endif

# Source files
SRV_SRC  := $(wildcard src/server/*.c) $(wildcard src/common/*.c)
CLI_SRC  := $(wildcard src/client/*.c) $(wildcard src/common/*.c)

# Platform detection for source exclusion
UNAME_OS := $(shell uname -s 2>/dev/null)

# Exclude platform-specific files
ifeq ($(OS),Windows_NT)
    SRV_SRC  := $(filter-out src/common/net_posix.c,$(SRV_SRC))
    CLI_SRC  := $(filter-out src/common/net_posix.c,$(CLI_SRC))
else ifeq ($(findstring MINGW,$(UNAME_OS)),MINGW)
    SRV_SRC  := $(filter-out src/common/net_posix.c,$(SRV_SRC))
    CLI_SRC  := $(filter-out src/common/net_posix.c,$(CLI_SRC))
else ifeq ($(findstring CYGWIN,$(UNAME_OS)),CYGWIN)
    SRV_SRC  := $(filter-out src/common/net_posix.c,$(SRV_SRC))
    CLI_SRC  := $(filter-out src/common/net_posix.c,$(CLI_SRC))
else
    SRV_SRC  := $(filter-out src/common/net_win.c,$(SRV_SRC))
    CLI_SRC  := $(filter-out src/common/net_win.c,$(CLI_SRC))
endif

# Debug mode
ifdef DEBUG
    OPT := -O0 -g
    ifeq ($(UNAME),Linux)
        OPT += -fsanitize=address,undefined
    endif
endif

CFLAGS := $(CSTD) $(OPT) $(INCLUDES)

# Targets
all: src/server/chat_server$(EXE) src/client/chat_client$(EXE)

src/server/chat_server$(EXE): $(SRV_SRC)
	@echo [BUILD] Server: $@
	$(CC) $(CFLAGS) $(CFLAGS_SRV) -o $@ $^ $(LDFLAGS_SRV)

src/client/chat_client$(EXE): $(CLI_SRC)
	@echo [BUILD] Client: $@
	$(CC) $(CFLAGS) $(CFLAGS_CLI) -o $@ $^ $(LDFLAGS_CLI)

clean:
	@echo [CLEAN]
	-rm -f src/server/chat_server$(EXE)
	-rm -f src/client/chat_client$(EXE)
	-rm -f src/server/*.o
	-rm -f src/client/*.o
	-rm -f src/common/*.o

.PHONY: all clean
