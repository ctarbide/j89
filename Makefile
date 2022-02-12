
SHELL = /bin/sh
CMP_S = cmp -s
INSTALL = ./tools/install.sh
NROFF = nroff -Tascii
SED = sed
MV_F = mv -f
RM_F = rm -f
CHMOD_0444 = chmod 444

PREFIX = $(HOME)/local
TMPDIR = /tmp
BINDIR = $(PREFIX)/bin
ETCDIR = $(PREFIX)/etc
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

DEFS += -DPCNONASCII=0xFF
DEFS += -DJOB_CONTROL
DEFS += -DNO_IPROCS

DEFS += -DCOMMANDS_SANITY_CHECK

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

PROGS = jove portsrv recover teachjove

.PHONY: all
all: maybe-update-jpaths $(PROGS)

.SUFFIXES: .o
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

jove.o: jpaths.h
recover.o: jpaths.h
teachjove.o: jpaths.h

setmaps.o: sysdep.h commands.tab vars.tab
commands.o: sysdep.h commands.tab vars.tab
vars.o: sysdep.h commands.tab vars.tab

jove: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

portsrv: portsrv.o
	$(CC) $(CFLAGS) -o $@ portsrv.o $(LIBS)

recover: recover.o
	$(CC) $(CFLAGS) -o $@ recover.o $(LIBS)

teachjove: teachjove.o
	$(CC) $(CFLAGS) -o $@ teachjove.o $(LIBS)

.PHONY: install
install: all doc/jove.rc doc/jove.doc doc/jove.1 doc/teachjove.1 doc/jovetool.1
	$(INSTALL) -D -m 555 jove '$(DESTDIR)$(BINDIR)/jove'
	$(INSTALL) -D -m 555 teachjove '$(DESTDIR)$(BINDIR)/teachjove'
	$(INSTALL) -D -m 555 recover '$(DESTDIR)$(LIBDIR)/recover'
	$(INSTALL) -D -m 555 portsrv '$(DESTDIR)$(LIBDIR)/portsrv'
	$(INSTALL) -D -m 444 doc/README '$(DESTDIR)$(SHAREDIR)/README'
	$(INSTALL) -D -m 444 doc/XTermresource '$(DESTDIR)$(SHAREDIR)/XTermresource'
	$(INSTALL) -D -m 444 doc/cmds.macros.nr '$(DESTDIR)$(SHAREDIR)/cmds.macros.nr'
	$(INSTALL) -D -m 444 doc/cmds.nr '$(DESTDIR)$(SHAREDIR)/cmds.nr'
	$(INSTALL) -D -m 444 doc/contents.nr '$(DESTDIR)$(SHAREDIR)/contents.nr'
	$(INSTALL) -D -m 444 doc/example.rc '$(DESTDIR)$(SHAREDIR)/example.rc'
	$(INSTALL) -D -m 444 doc/intro.nr '$(DESTDIR)$(SHAREDIR)/intro.nr'
	$(INSTALL) -D -m 444 doc/jove.nr '$(DESTDIR)$(SHAREDIR)/jove.nr'
	$(INSTALL) -D -m 444 doc/jove.qref '$(DESTDIR)$(SHAREDIR)/jove.qref'
	$(INSTALL) -D -m 444 doc/jove.rc.3022 '$(DESTDIR)$(SHAREDIR)/jove.rc.3022'
	$(INSTALL) -D -m 444 doc/jove.rc.in '$(DESTDIR)$(SHAREDIR)/jove.rc.in'
	$(INSTALL) -D -m 444 doc/jove.rc.sun '$(DESTDIR)$(SHAREDIR)/jove.rc.sun'
	$(INSTALL) -D -m 444 doc/jove.rc.sun-cmd '$(DESTDIR)$(SHAREDIR)/jove.rc.sun-cmd'
	$(INSTALL) -D -m 444 doc/jove.rc.vt100 '$(DESTDIR)$(SHAREDIR)/jove.rc.vt100'
	$(INSTALL) -D -m 444 doc/jove.rc.wyse '$(DESTDIR)$(SHAREDIR)/jove.rc.wyse'
	$(INSTALL) -D -m 444 doc/jove.rc.xterm '$(DESTDIR)$(SHAREDIR)/jove.rc.xterm'
	$(INSTALL) -D -m 444 doc/jove.rc.xterm-256color '$(DESTDIR)$(SHAREDIR)/jove.rc.xterm-256color'
	$(INSTALL) -D -m 444 doc/jove.rc.z29 '$(DESTDIR)$(SHAREDIR)/jove.rc.z29'
	$(INSTALL) -D -m 444 doc/jovetool.nr '$(DESTDIR)$(SHAREDIR)/jovetool.nr'
	$(INSTALL) -D -m 444 doc/keychart. '$(DESTDIR)$(SHAREDIR)/keychart.'
	$(INSTALL) -D -m 444 doc/keychart.3022 '$(DESTDIR)$(SHAREDIR)/keychart.3022'
	$(INSTALL) -D -m 444 doc/keychart.sun '$(DESTDIR)$(SHAREDIR)/keychart.sun'
	$(INSTALL) -D -m 444 doc/keychart.sun-cmd '$(DESTDIR)$(SHAREDIR)/keychart.sun-cmd'
	$(INSTALL) -D -m 444 doc/keychart.vt100 '$(DESTDIR)$(SHAREDIR)/keychart.vt100'
	$(INSTALL) -D -m 444 doc/keychart.wyse '$(DESTDIR)$(SHAREDIR)/keychart.wyse'
	$(INSTALL) -D -m 444 doc/keychart.xterm '$(DESTDIR)$(SHAREDIR)/keychart.xterm'
	$(INSTALL) -D -m 444 doc/keychart.xterm-256color '$(DESTDIR)$(SHAREDIR)/keychart.xterm-256color'
	$(INSTALL) -D -m 444 doc/keychart.z29 '$(DESTDIR)$(SHAREDIR)/keychart.z29'
	$(INSTALL) -D -m 444 doc/teach-jove '$(DESTDIR)$(SHAREDIR)/teach-jove'
	$(INSTALL) -D -m 444 doc/teachjove.nr '$(DESTDIR)$(SHAREDIR)/teachjove.nr'
	$(INSTALL) -D -m 444 doc/xjove.nr '$(DESTDIR)$(SHAREDIR)/xjove.nr'
	$(INSTALL) -D -m 444 doc/jove.rc '$(DESTDIR)$(SHAREDIR)/jove.rc'
	$(INSTALL) -D -m 444 doc/jove.doc '$(DESTDIR)$(SHAREDIR)/jove.doc'
	$(INSTALL) -D -m 444 doc/jove.1 '$(DESTDIR)$(MANDIR)/jove.1'
	$(INSTALL) -D -m 444 doc/teachjove.1 '$(DESTDIR)$(MANDIR)/teachjove.1'
	$(INSTALL) -D -m 444 doc/jovetool.1 '$(DESTDIR)$(MANDIR)/jovetool.1'
	$(INSTALL) -D -m 444 doc/xjove.nr '$(DESTDIR)$(MANDIR)/xjove.1'

.PHONY: maybe-update-jpaths
maybe-update-jpaths:
	@echo "/* Changes should be made in Makefile, not to this file! */" >jpaths.tmp
	@echo "" >>jpaths.tmp
	@echo \#define TMPDIR \"$(TMPDIR)\" >>jpaths.tmp
	@echo \#define RECDIR \"$(JRECDIR)\" >>jpaths.tmp
	@echo \#define LIBDIR \"$(LIBDIR)\" >>jpaths.tmp
	@echo \#define SHAREDIR \"$(SHAREDIR)\" >>jpaths.tmp
	@echo \#define DFLTSHELL \"$(DFLTSHELL)\" >>jpaths.tmp
	@set -x; if ! $(CMP_S) jpaths.h jpaths.tmp 2>/dev/null; \
		then $(MV_F) jpaths.tmp jpaths.h; $(CHMOD_0444) jpaths.h; \
		else $(RM_F) jpaths.tmp; \
	fi

doc/jove.rc: doc/jove.rc.in
	$(SED) "s,__ETCDIR__,$(ETCDIR)," doc/jove.rc.in >doc/jove.rc.tmp
	if ! $(CMP_S) doc/jove.rc.tmp doc/jove.rc 2>/dev/null; \
		then $(MV_F) doc/jove.rc.tmp doc/jove.rc; $(CHMOD_0444) jpaths.h; \
		else $(RM_F) doc/jove.rc.tmp; \
	fi

setmaps: setmaps.o
	$(CC) $(CFLAGS) -o $@ setmaps.o

keys.c: setmaps keys.txt
	./setmaps <keys.txt >keys.tmp
	$(MV_F) keys.tmp $@
	$(CHMOD_0444) $@

doc/jove.doc: doc/jove.nr
	@-$(RM_F) doc/jove.doc
	LANG=C $(NROFF) -man doc/jove.nr >doc/jove.doc

doc/jove.1: doc/jove.nr
	@-$(RM_F) doc/jove.1
	$(SED) -e 's;<TMPDIR>;$(TMPDIR);' \
		-e 's;<LIBDIR>;$(LIBDIR);' \
		-e 's;<SHAREDIR>;$(SHAREDIR);' \
		-e 's;<SHELL>;$(DFLTSHELL);' \
		doc/jove.nr >doc/jove.1

doc/teachjove.1: doc/teachjove.nr
	@-$(RM_F) doc/teachjove.1
	$(SED) -e 's;<TMPDIR>;$(JTMPDIR);' \
		-e 's;<LIBDIR>;$(JLIBDIR);' \
		-e 's;<SHAREDIR>;$(JSHAREDIR);' \
		-e 's;<SHELL>;$(DFLTSHELL);' \
		doc/teachjove.nr >doc/teachjove.1

doc/jovetool.1: doc/jovetool.nr
	@-$(RM_F) doc/jovetool.1
	$(SED) -e 's;<MANDIR>;$(MANDIR);' \
		-e 's;<MANEXT>;1;' \
		doc/jovetool.nr > doc/jovetool.1

clean:
	$(RM_F) $(OBJS)
	$(RM_F) $(OBJS_OTHERS) $(OBJS_TEACHJOVE)
	$(RM_F) $(PROGS) jpaths.h keys.c
