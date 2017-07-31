
OS_NAME=$(shell uname)

ifeq ($(OS), Windows_NT)
  CC=clang
  SO_EXT=.dll
  A_EXT=.lib
  OPT_FLAGS=-m32
  SUB_DIR=windows
endif
ifeq ($(OS_NAME), Darwin)
  CC=clang
  SO_EXT=.dylib
  A_EXT=.a
  OPT_FLAGS=
  SUB_DIR=darwin
endif
ifeq ($(OS_NAME), Linux)
  CC=gcc
  SO_EXT=.so
  A_EXT=.a
  OPT_FLAGS=-Wl,-soname,libsmr$(SO_EXT)
  SUB_DIR=linux
endif

#this should work on Linux, Mac and Windows with Cygwin
RM=rm -f

CFLAGS=-g -Wall -fPIC

PREFIX=./lib/$(SUB_DIR)/libsmr

#NOTE: the call to 'ar' probably isn't necessary as we only have
#      a single object file

all: shared static

shared:
	$(CC) -o $(PREFIX)$(SO_EXT) $(CFLAGS) -shared $(OPT_FLAGS) smr.c

static:
	$(CC) -o $(PREFIX).o $(CFLAGS) -c smr.c
	ar rcs $(PREFIX)$(A_EXT) $(PREFIX).o

clean:
	$(RM) $(PREFIX)$(SO_EXT) $(PREFIX)$(A_EXT) $(PREFIX).o
