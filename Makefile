
INSTALL = ./tools/install.sh

PREFIX = $(HOME)/local
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib/jove
SHAREDIR = $(PREFIX)/share/jove
MAN1DIR = $(PREFIX)/share/man/man1

CC = gcc
CFLAGS = -std=c99 -g -O2 -Wall -I${HOME}/local/include
# CFLAGS += -Werror -fmax-errors=3
LIBS = -L${HOME}/local/lib64 -L${HOME}/local/lib
LIBS += -lm -lncursesw -lutil

# OPTFLAGS = -ggdb3 -O0
OPTFLAGS = -g -O2
WERROR = -pedantic -Werror -fmax-errors=5
CFLAGS_QA = -std=c99 $(OPTFLAGS) \
    -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes \
    -Wshadow -Wconversion -Wdeclaration-after-statement \
    -Wno-unused-parameter \
    $(WERROR) \
    -I${HOME}/local/include
CFLAGS = $(CFLAGS_QA)

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
    jtc.o \
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
    portsrv.o \
    proc.o \
    reapp.o \
    re.o \
    rec.o \
    recover.o \
    scandir.o \
    screen.o \
    setmaps.o \
    teachjove.o \
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
    jtc.c \
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
    portsrv.c \
    proc.c \
    reapp.c \
    re.c \
    rec.c \
    recover.c \
    scandir.c \
    screen.c \
    setmaps.c \
    teachjove.c \
    term.c \
    termcap.c \
    unix.c \
    util.c \
    vars.c \
    win32.c \
    wind.c

all: jove

.SUFFIXES: .o
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

jove: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

protos.h: *.nw
	nofake.sh -L -Rprotos -oprotos.h *.nw

.PHONY: install
install: jove
	$(INSTALL) -D -m 755 jove '$(BINDIR)/jove'

clean:
	rm -f $(OBJS) jove
