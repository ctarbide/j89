/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"
#include "chars.h"
#include "fp.h"
#include "jctype.h"
#include "disp.h"
#include "extend.h"
#include "fmt.h"

#ifdef MAC
# include  "mac.h"
#endif

private void
doformat proto((File *, const char *, va_list)),
	 pad proto((DAPchar, int));

char	mesgbuf[MESG_SIZE];

/* Formatting codes supported:
 *
 * %%: => '%'
 * %O, %D, %X: long => octal, decimal, or hex
 * %lo, %ld, %lx: long => octal, decimal, or hex
 * %o, %d, %x: int => octal, decimal, or hex
 * %c: char => character
 * %s: char* => string
 *
 * %b: buffer pointer => buffer's name
 * %f: void => current command's name
 * %n: int => int == 1? "" : "s"
 * %p: char => visible rep
 */

void
format(char *buf, size_t len, const char *fmt, va_list ap)
{
	File	strbuf;
	if (len > INT_MAX) {
		fprintf(stderr, "fatal: %s:%d: len > INT_MAX\n", __FILE__, __LINE__);
		exit(1);
	}
	strbuf.f_ptr = strbuf.f_base = buf;
	strbuf.f_fd = -1;		/* Not legit for files */
	strbuf.f_bufsize = strbuf.f_cnt = (int)len;
	strbuf.f_flags = F_STRING;
	doformat(&strbuf, fmt, ap);
	f_putc('\0', &strbuf);	/* f_putc will place this, even if overflow */
}

/* pretty-print character c into buffer cp (up to PPWIDTH bytes) */

void
PPchar(ZXchar c, char *cp)
{
	if (jisprint(c)) {
		cp[0] = (char)c;
		cp[1] = '\0';
	} else if (c < DEL) {
		strcpy(cp, "^?");
		cp[1] = (char)(c + '@');
	} else if (c == DEL) {
		strcpy(cp, "^?");
	} else {
		cp[0] = '\\';
		cp[1] = (char)('0' + (c >> 6));
		cp[2] = (char)('0' + ((c >> 3) & 07));
		cp[3] = (char)('0' + (c & 07));
		cp[4] = '\0';
	}
}

private struct fmt_state {
	int	precision,
		width;
	bool	leftadj;
	char	padc;
	File	*iop;
} current_fmt;

/* TODO: Make this unsigned long when we dump support for pre-ANSI C (use flag to ask for sign?) */
private void
putld(long d, int base)
{
	static const char	chars[] = {'0', '1', '2', '3', '4', '5', '6',
			'7', '8', '9', 'a', 'b', 'c', 'd',
			'e', 'f'
		};
	int	len = 0;
	long	tmpd = d;
	char	ubuf[32],
		*ep = ubuf + sizeof(ubuf),
		 *up = ep;

	if (d < 0) {
		len += 1;
		tmpd = -d;
	}

	if (current_fmt.width == 0 && current_fmt.precision) {
		current_fmt.width = current_fmt.precision;
		current_fmt.padc = '0';
	}

	do {
		int i = (int)(tmpd % base);
		tmpd = tmpd / base;
		*--up = chars[i];
		len += 1;
	} while (tmpd != 0);

	if (!current_fmt.leftadj) {
		pad(current_fmt.padc, current_fmt.width - len);
	}

	if (d < 0) {
		f_putc('-', current_fmt.iop);
	}

	while (up != ep) {
		f_putc((int)*up, current_fmt.iop);
		up++;
	}

	if (current_fmt.leftadj) {
		pad(current_fmt.padc, current_fmt.width - len);
	}
}

private void
fmt_puts(const char *str)
{
	int	len;
	const char	*cp;

	if (str == NULL) {
		str = "(null)";
	}

	len = (int)strlen(str);

	if (current_fmt.precision == 0 || len < current_fmt.precision) {
		current_fmt.precision = len;
	} else {
		len = current_fmt.precision;
	}

	cp = str;

	if (!current_fmt.leftadj) {
		pad(' ', current_fmt.width - len);
	}

	while (--current_fmt.precision >= 0) {
		f_putc(*cp++, current_fmt.iop);
	}

	if (current_fmt.leftadj) {
		pad(' ', current_fmt.width - len);
	}
}

private void
pad(DAPchar c, int amount)
{
	while (c && --amount >= 0) {
		f_putc((char)c, current_fmt.iop);
	}
}

private void
doformat(File *sp, const char *fmt, va_list ap)
{
	char	c;
	struct fmt_state	prev_fmt;
	prev_fmt = current_fmt;
	current_fmt.iop = sp;

	while ((c = *fmt++) != '\0') {
		if (c != '%') {
			f_putc(c, current_fmt.iop);
			continue;
		}

		current_fmt.padc = ' ';
		current_fmt.precision = current_fmt.width = 0;
		current_fmt.leftadj = NO;
		c = *fmt++;

		if (c == '-') {
			current_fmt.leftadj = YES;
			c = *fmt++;
		}

		if (c == '0') {
			current_fmt.padc = '0';
			c = *fmt++;
		}

		while (c >= '0' && c <= '9') {
			current_fmt.width = current_fmt.width * 10 + (c - '0');
			c = *fmt++;
		}

		if (c == '*') {
			current_fmt.width = va_arg(ap, int);
			c = *fmt++;
		}

		if (c == '.') {
			c = *fmt++;

			while (c >= '0' && c <= '9') {
				current_fmt.precision = current_fmt.precision * 10 + (c - '0');
				c = *fmt++;
			}

			if (c == '*') {
				current_fmt.precision = va_arg(ap, int);
				c = *fmt++;
			}
		}

reswitch:

		/* At this point, fmt points at one past the format letter. */
		switch (c) {
		case '%':
			f_putc('%', current_fmt.iop);
			break;

		case 'O':
		case 'D':
		case 'X':
			putld(va_arg(ap, long), (c == 'O') ? 8 :
				(c == 'D') ? 10 : 16);
			break;

		case 'b': {
			Buffer	*b = va_arg(ap, Buffer *);
			fmt_puts(b->b_name);
			break;
		}

		case 'c':
			f_putc((char)va_arg(ap, DAPchar), current_fmt.iop);
			break;

		case 'o':
		case 'd':
		case 'x':
			putld((long) va_arg(ap, int), (c == 'o') ? 8 :
				(c == 'd') ? 10 : 16);
			break;

		case 'f':	/* current command name gets inserted here! */
			fmt_puts(LastCmd->Name);
			break;

		case 'l':
			c = (char)CharUpcase(*++fmt);
			goto reswitch;

		case 'n':
			if (va_arg(ap, int) != 1) {
				fmt_puts("s");
			}

			break;

		case 'p': {
			ZXchar	cc = ZXC(va_arg(ap, DAPchar));

			if (cc == ESC) {
				fmt_puts("ESC");
			} else {
				char	cbuf[PPWIDTH];
				PPchar(cc, cbuf);
				fmt_puts(cbuf);
			}
		}
		break;

		case 's':
			fmt_puts(va_arg(ap, char *));
			break;

		default:
			complain("Unknown format directive: \"%%%c\"", c);
			/* NOTREACHED */
		}
	}

	current_fmt = prev_fmt;
}

#ifdef STDARGS
char *
sprint(const char *fmt, ...)
#else
/*VARARGS1*/ char *
sprint(fmt, va_alist)
const char	*fmt;
va_dcl
#endif
{
	va_list	ap;
	static char	line[LBSIZE];

	va_init(ap, fmt);
	format(line, sizeof line, fmt, ap);
	va_end(ap);
	return line;
}

#ifdef STDARGS
void
writef(const char *fmt, ...)
#else
/*VARARGS1*/ void
writef(fmt, va_alist)
const char	*fmt;
va_dcl
#endif
{
	va_list	ap;

	va_init(ap, fmt);
#ifdef NO_JSTDOUT
	/* Can't use sprint because caller might have
	 * passed the result of sprint as an arg.
	 */
	{
		char	line[100];

		format(line, sizeof(line), fmt, ap);
		putstr(line);
	}
#else /* !NO_JSTDOUT */
	doformat(jstdout, fmt, ap);
#endif /* !NO_JSTDOUT */
	va_end(ap);
}

#ifdef STDARGS
void
fwritef(File *fp, const char *fmt, ...)
#else
/*VARARGS2*/ void
fwritef(fp, fmt, va_alist)
File	*fp;
const char	*fmt;
va_dcl
#endif
{
	va_list	ap;

	va_init(ap, fmt);
	doformat(fp, fmt, ap);
	va_end(ap);
}

#ifdef STDARGS
void
swritef(char *str, size_t size, const char *fmt, ...)
#else
/*VARARGS3*/ void
swritef(str, size, fmt, va_alist)
char	*str;
size_t	size;
const char	*fmt;
va_dcl
#endif
{
	va_list	ap;

	va_init(ap, fmt);
	format(str, size, fmt, ap);
	va_end(ap);
}

/* send a message (supressed if input pending) */

#ifdef STDARGS
void
s_mess(const char *fmt, ...)
#else
/*VARARGS1*/ void
s_mess(fmt, va_alist)
const char	*fmt;
va_dcl
#endif
{
	va_list	ap;

	if (InJoverc)
	{
		return;
	}

	va_init(ap, fmt);
	format(mesgbuf, sizeof mesgbuf, fmt, ap);
	va_end(ap);
	message(mesgbuf);
}

/* force a message: display it now no matter what.
 * If you wish it to stick, set stickymsg on after calling f_mess.
 */

#ifdef STDARGS
void
f_mess(const char *fmt, ...)
#else
/*VARARGS1*/ void
f_mess(fmt, va_alist)
const char	*fmt;
va_dcl
#endif
{
	va_list	ap;

	va_init(ap, fmt);
	format(mesgbuf, sizeof mesgbuf, fmt, ap);
	va_end(ap);
	DrawMesg(NO);
	stickymsg = NO;
	UpdMesg = YES;	/* still needs updating (for convenience) */
}

void
add_mess(const char *fmt, ...)
{
	size_t	mesg_len = strlen(mesgbuf);
	va_list	ap;

	if (InJoverc) {
		return;
	}

	va_init(ap, fmt);
	format(&mesgbuf[mesg_len], sizeof(mesgbuf) - mesg_len, fmt, ap);
	va_end(ap);
	message(mesgbuf);
}

bool jdebug	    = YES;  /* so that first jdprintf is called */
const char *jdpath  = NULL; /* if non-NULL, will be opened on first jdprintf */

#ifdef STDARGS
void
jdprintf(const char *fmt, ...)
#else
/*VARARGS1*/ void
jdprintf(fmt, va_alist)
const char	*fmt;
va_dcl
#endif
{
	static bool first_time = YES;
	static File *dfp = NULL;
	va_list	ap;

	if (first_time && jdpath != NULL)
	{
		dfp = f_open(jdpath, F_WRITE | F_LOCKED, NULL, LBSIZE);
		jdebug = (dfp != NULL);
		first_time = NO;
	}

	va_init(ap, fmt);

	if (dfp != NULL)
	{
		doformat(dfp, fmt, ap);
		flushout(dfp);
	}

	va_end(ap);
}
