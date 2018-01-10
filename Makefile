
OS_NAME=$(shell uname)
EXE_EXT=
ifeq ($(OS), Windows_NT)
  CC=clang
  SO_EXT=.dll
  A_EXT=.lib
  EXE_EXT=.exe
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

smr2mda: static
	$(CC) -o ./bin/smr2mda$(EXE_EXT) $(CFLAGS) smr2mda.c $(PREFIX).o -lm

shared: smr.c smr.h smr_utilities.h
	$(CC) -o $(PREFIX)$(SO_EXT) $(CFLAGS) -shared $(OPT_FLAGS) smr.c

static: smr.c smr.h smr_utilities.h
	$(CC) -o $(PREFIX).o $(CFLAGS) -c smr.c
	ar rcs $(PREFIX)$(A_EXT) $(PREFIX).o

clean:
	$(RM) $(PREFIX)$(SO_EXT) $(PREFIX)$(A_EXT) $(PREFIX).o smr2mda.*
