
SHELL = /bin/sh
CMP = cmp
INSTALL = ./tools/install.sh

PREFIX = $(HOME)/local
TMPDIR = /var/tmp
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib/jove
SHAREDIR = $(PREFIX)/share/jove
MANDIR = $(PREFIX)/share/man/man1

# SHAREDIR is for online documentation, and the distributed standard system-wide
# jove.rc file with some common
#
# LIBDIR is for the PORTSRV and RECOVER programs.
#
# BINDIR is where to put the executables JOVE and TEACHJOVE.
#
# MANDIR is where the manual pages go for JOVE, RECOVER and TEACHJOVE.
#
# TMPDIR is where the tmp files get stored, usually /tmp, /var/tmp, or
# /usr/tmp.  If you wish to be able to recover buffers after a system
# crash, this needs to be a directory that isn't cleaned out on reboot.
# You would probably want to clean out that directory periodically with
# /etc/cron.
#
# JRECDIR is the directory in which RECOVER looks for JOVE's tempfiles
# (in case the system startup salvages tempfiles by moving them,
# which is probably a good idea).
#
# DFLTSHELL is the default shell invoked by JOVE and TEACHJOVE.
#

JRECDIR = /var/lib/jove/preserve
DFLTSHELL = $(SHELL)

DEFS = -DUSE_STDIO_H
DEFS += -DREALSTDC
DEFS += -DF_COMPLETION
DEFS += -DFULL_UNISTD
DEFS += -DUSE_GETCWD

DEFS += -D_DEFAULT_SOURCE
DEFS += -D_ISOC99_SOURCE
DEFS += -D_SVID_SOURCE
DEFS += -D_BSD_SOURCE
DEFS += -D_XOPEN_SOURCE=700
DEFS += -D_XOPEN_SOURCE_EXTENDED
DEFS += -D_POSIX_C_SOURCE=200809L

DEFS += -DUSE_LIMITS_H
DEFS += -DUSE_STDINT_H

DEFS += -DTERMIOS
DEFS += -DHAVE_SPEED_T

# 32-bit
# DEFS += -DINTPTR_T=int
# DEFS += -DUINTPTR_T="unsigned int"
# DEFS += -DSSIZE_T=int

# 64-bit
# DEFS += -DINTPTR_T=long
# DEFS += -DUINTPTR_T="unsigned long"
# DEFS += -DSSIZE_T=long

CC = gcc
CFLAGS = -ansi -pedantic -g -O2 -Wall -I${HOME}/local/include
# CFLAGS += -Werror -fmax-errors=3
LIBS = -L${HOME}/local/lib64 -L${HOME}/local/lib
LIBS += -lm -lncursesw -lutil

OPTFLAGS = -ggdb3 -O0
# OPTFLAGS = -g -O2

WERROR = -Werror -fmax-errors=5
CFLAGS_QA = -ansi -pedantic $(OPTFLAGS) \
    -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes \
    -Wshadow -Wconversion -Wdeclaration-after-statement \
    -Wno-unused-parameter \
    $(WERROR) \
    -I${HOME}/local/include
CFLAGS = $(CFLAGS_QA)

CFLAGS += $(DEFS)

OBJS = \
    abbrev.o \
    argcount.o \
    ask.o \
    buf.o \
    case.o \
    c.o \
    commands.o \
    delete.o \
    disp.o \
    extend.o \
    fmt.o \
    fp.o \
    ibmpcdos.o \
    insert.o \
    io.o \
    iproc.o \
    jctype.o \
    jove.o \
    keymaps.o \
    keys.o \
    list.o \
    mac.o \
    macros.o \
    marks.o \
    misc.o \
    mouse.o \
    move.o \
    msgetch.o  \
    para.o \
    proc.o \
    reapp.o \
    re.o \
    rec.o \
    scandir.o \
    screen.o \
    term.o \
    termcap.o \
    unix.o \
    util.o \
    vars.o \
    win32.o \
    wind.o

SRCS = \
    abbrev.c \
    argcount.c \
    ask.c \
    buf.c \
    case.c \
    c.c \
    commands.c \
    delete.c \
    disp.c \
    extend.c \
    fmt.c \
    fp.c \
    ibmpcdos.c \
    insert.c \
    io.c \
    iproc.c \
    jctype.c \
    jove.c \
    keymaps.c \
    keys.c \
    list.c \
    mac.c \
    macros.c \
    marks.c \
    misc.c \
    mouse.c \
    move.c \
    msgetch.c  \
    para.c \
    proc.c \
    reapp.c \
    re.c \
    rec.c \
    scandir.c \
    screen.c \
    term.c \
    termcap.c \
    unix.c \
    util.c \
    vars.c \
    win32.c \
    wind.c

OBJS_TEACHJOVE = \
    teachjove.o

SRCS_TEACHJOVE = \
    teachjove.c

OBJS_OTHERS = \
    jtc.o \
    portsrv.o \
    recover.o \
    setmaps.o

SRCS_OTHERS = \
    jtc.c \
    portsrv.c \
    recover.c \
    setmaps.c

all: jove

.SUFFIXES: .o
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

jove.o: jpaths.h
recover.o: jpaths.h
teachjove.o: jpaths.h

jove: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

protos.h: *.nw
	nofake.sh -L -Rprotos -oprotos.h *.nw

.PHONY: install
install: jove
	$(INSTALL) -D -m 755 jove '$(DESTDIR)$(BINDIR)/jove'

jpaths.h: Makefile
	@-rm -f jpaths.tmp
	@echo "/* Changes should be made in Makefile, not to this file! */" > jpaths.tmp
	@echo "" >> jpaths.tmp
	@echo \#define TMPDIR \"$(TMPDIR)\" >> jpaths.tmp
	@echo \#define RECDIR \"$(JRECDIR)\" >> jpaths.tmp
	@echo \#define LIBDIR \"$(LIBDIR)\" >> jpaths.tmp
	@echo \#define SHAREDIR \"$(SHAREDIR)\" >> jpaths.tmp
	@echo \#define DFLTSHELL \"$(DFLTSHELL)\" >> jpaths.tmp
	if ! $(CMP) -s $@ jpaths.tmp 2> /dev/null; then mv jpaths.tmp $@; else rm jpaths.tmp; fi

setmaps: setmaps.o
	$(CC) $(CFLAGS) -o $@ setmaps.o

keys.c: setmaps keys.txt
	@-rm -f keys.c
	./setmaps < keys.txt > keys.c

clean:
	rm -f $(OBJS) jove jpaths.h
