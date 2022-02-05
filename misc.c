/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"
#include "jctype.h"
#include "disp.h"
#include "ask.h"
#include "c.h"
#include "delete.h"
#include "insert.h"
#include "extend.h"
#include "fmt.h"
#include "marks.h"
#include "misc.h"
#include "move.h"
#include "para.h"

void
prCTIME(void)
{
	f_mess(": %f %s", get_time((time_t *)NULL, (char *)NULL, 0, -1));
	stickymsg = YES;
}

void
ChrToOct(void)
{
	ZXchar	c = ask_ks();
	ins_str(sprint("\\%03o", c));
#ifdef PCNONASCII

	if (c == PCNONASCII) {
		c = waitchar();
		ins_str(sprint("\\%03o", c));
	}

#endif
}

void
StrLength(void)
{
	static const char	inquotes[] = "Where are the quotes?";
	char	*cp;
	int	numchars = 0;

	for (cp = linebuf + curchar; ; cp--) {
		if (*cp == '"' && (cp == linebuf || cp[-1] != '\\')) {
			break;
		}

		if (cp == linebuf) {
			complain(inquotes);
			/* NOTREACHED */
		}
	}

	cp += 1;	/* skip opening quote */

	for (;;) {
		switch (*cp++) {
		case '\0':
			complain(inquotes);

		/*NOTREACHED*/
		case '"':
			f_mess("%d characters", numchars);
			stickymsg = YES;
			return;

		case '\\':
			if (!jisdigit(*cp)) {
				if (*cp == '\0') {
					complain(inquotes);
					/* NOTREACHED */
				}

				cp += 1;
			} else {
				int	num = 3;

				do {
					cp += 1;
				} while (--num != 0 && jisdigit(*cp));
			}

			break;
		}

		numchars += 1;
	}
}

/* Transpose cur_char with cur_char - 1 */

void
TransChar(void)
{
	char	before;

	if (curchar == 0 || (eolp() && curchar == 1)) {
		complain((char *)NULL);	/* BEEP */
		/* NOTREACHED */
	}

	if (eolp()) {
		b_char(1);
	}

	before = linebuf[curchar - 1];
	del_char(BACKWARD, 1, NO);
	f_char(1);
	insert_c(before, 1);
}

/* Switch current line with previous one */

void
TransLines(void)
{
	daddr	old_prev;

	if (firstp(curline)) {
		return;
	}

	lsave();
	/* Exchange l_dline values.
	 * CHEAT: this breaks the buffer abstraction.
	 * The getDOT unfools a few caching mechanisms.
	 */
	old_prev = curline->l_prev->l_dline;
	curline->l_prev->l_dline = curline->l_dline;
	curline->l_dline = old_prev;
	getDOT();

	if (!lastp(curline)) {
		line_move(FORWARD, 1, NO);
	} else {
		Eol();        /* can't move to next line, so we do the next best thing */
	}

	modify();
	DOLsave = NO;	/* CHEAT: contents of linebuf need not override l_dline. */
}

/* exit-jove command */

void
Leave(void)
{
	longjmp(mainjmp, JMP_QUIT);
	/* NOTREACHED */
}

/* If argument is specified, kill that many lines down.  Otherwise,
 * if we "appear" to be at the end of a line, i.e. everything to the
 * right of the cursor is white space, we delete the line separator
 * as if we were at the end of the line.
 */
void
KillEOL(void)
{
	LinePtr	line2;
	int	char2;
	long	num = arg_value();

	if (is_an_arg()) {
		if (num == 0) {	/* Kill to beginning of line */
			line2 = curline;
			char2 = 0;
		} else {
			line2 = next_line(curline, num);

			if ((LineDist(curline, line2) < num) || (line2 == curline)) {
				char2 = length(line2);
			} else {
				char2 = 0;
			}
		}
	} else if (blnkp(&linebuf[curchar])) {
		line2 = next_line(curline, 1);

		if (line2 == curline) {
			char2 = length(curline);
		} else {
			char2 = 0;
		}
	} else {
		line2 = curline;
		char2 = length(curline);
	}

	reg_kill(line2, char2, NO);
}

/* kill to beginning of sentence */

void
KillBos(void)
{
	negate_arg();
	KillEos();
}

/* Kill to end of sentence */

void
KillEos(void)
{
	LinePtr	line1;
	int	char1;
	line1 = curline;
	char1 = curchar;
	Eos();
	reg_kill(line1, char1, YES);
}

void
KillExpr(void)
{
	LinePtr	line1;
	int	char1;
	line1 = curline;
	char1 = curchar;
	FSexpr();
	reg_kill(line1, char1, YES);
}

void
Yank(void)
{
	LinePtr	line,
		lp;
	Bufpos	*dot;

	if (killbuf[killptr] == NULL) {
		complain("[Nothing to yank!]");
		/* NOTREACHED */
	}

	lsave();
	line = killbuf[killptr];
	lp = lastline(line);
	dot = DoYank(line, 0, lp, length(lp), curline, curchar, curbuf);
	set_mark();
	SetDot(dot);
}

void
ToIndent(void)
{
	Bol();
	skip_wht_space();
}

void
skip_wht_space(void)
{
	INTPTR_T	diff;
	char		*cp = linebuf + curchar;

	while (jiswhite(*cp)) {
		cp += 1;
	}

	diff = cp - linebuf;

	if ((UINTPTR_T)diff > INT_MAX) {
		fprintf(stderr, "fatal: %s:%d: diff > INT_MAX\n", __FILE__, __LINE__);
		exit(1);
	}

	curchar = (int)diff;
}

/* GoLine -- go to a line, usually wired to goto-line, ESC g or ESC G.
 * If no argument is specified it asks for a line number.
 */
void
GoLine(void)
{
	LinePtr	newline;

	if (!is_an_arg()) {
		set_arg_value(ask_long("1", "Line: ", 10));
	}

	if (arg_value() < 0) {
		newline = prev_line(curbuf->b_last, -1 - arg_value());
	} else {
		newline = next_line(curbuf->b_first, arg_value() - 1);
	}

	PushPntp(newline);
	SetLine(newline);
}

void
NotModified(void)
{
	unmodify();
}

void
SetLMargin(void)
{
	int	lmarg = calc_pos(linebuf, curchar);

	if (lmarg >= RMargin) {
		complain("[Left margin must be left of right margin]");
		/* NOTREACHED */
	}

	LMargin = lmarg;
}

void
SetRMargin(void)
{
	int	rmarg = calc_pos(linebuf, curchar);

	if (rmarg <= LMargin) {
		complain("[Right margin must be right of left margin]");
		/* NOTREACHED */
	}

	RMargin = rmarg;
}
