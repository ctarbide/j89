/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"
#include "re.h"
#include "chars.h"
#include "jctype.h"
#include "disp.h"
#include "move.h"
#include "screen.h"	/* for tabstop */

private int	line_pos;

void
f_char(long n)
{
	if (n < 0) {
		b_char(-n);
		return;
	}

	while (--n >= 0) {
		if (eolp()) {			/* Go to the next Line */
			if (curline->l_next == NULL) {
				break;
			}

			SetLine(curline->l_next);
		} else {
			curchar += 1;
		}
	}
}

void
b_char(long n)
{
	if (n < 0) {
		f_char(-n);
		return;
	}

	while (--n >= 0) {
		if (bolp()) {
			if (curline->l_prev == NULL) {
				break;
			}

			SetLine(curline->l_prev);
			Eol();
		} else {
			curchar -= 1;
		}
	}
}

void
ForChar(void)
{
	f_char(arg_value());
}

void
BackChar(void)
{
	b_char(arg_value());
}

void
NextLine(void)
{
	if ((curline == curbuf->b_last) && eolp()) {
		complain(NullStr);
		/* NOTREACHED */
	}

	line_move(FORWARD, arg_value(), YES);
}

void
PrevLine(void)
{
	if ((curline == curbuf->b_first) && bolp()) {
		complain(NullStr);
		/* NOTREACHED */
	}

	line_move(BACKWARD, arg_value(), YES);
}

/* moves to a different line in DIR; LINE_CMD says whether this is
 * being called from NextLine() or PrevLine(), in which case it tries
 * to line up the column with the column of the current line
 */
void
line_move(int dir, long n, bool line_cmd)
{
	LinePtr(*proc) ptrproto((LinePtr, long)) =
		(dir == FORWARD) ? next_line : prev_line;
	LinePtr	line;
	line = (*proc)(curline, n);

	if (line == curline) {
		if (dir == FORWARD) {
			Eol();
		} else {
			Bol();
		}

		return;
	}

	if (line_cmd) {
		this_cmd = LINECMD;

		if (last_cmd != LINECMD) {
			line_pos = calc_pos(linebuf, curchar);
		}
	}

	SetLine(line);		/* curline is in linebuf now */

	if (line_cmd) {
		curchar = how_far(curline, line_pos);
	}
}

/* how_far returns what cur_char should be to be at or beyond col
 * screen columns in to the line.
 *
 * Note: if col indicates a position in the middle of a Tab or other
 * extended character, the result corresponds to that character
 * (as if col had indicated its start).
 *
 * Note: the calc_pos, how_far, and DeTab must be in synch --
 * each thinks it knows how characters are displayed.
 */

int
how_far(LinePtr line, int col)
{
	char	*lp;
	int	pos;
	ZXchar	c;
	char	*base;
	INTPTR_T	diff;
	base = lp = lcontents(line);
	pos = 0;

	do {
		if ((c = ZXC(*lp)) == '\t' && tabstop != 0) {
			pos += TABDIST(pos);
		} else if (jisprint(c)) {
			pos += 1;
		} else {
			if (c <= DEL) {
				pos += 2;
			} else {
				pos += 4;
			}
		}

		lp += 1;
	} while (pos <= col && c != '\0');

	diff = lp - base - 1;

	if ((UINTPTR_T)diff > INT_MAX) {
		fprintf(stderr, "fatal: %s:%d: diff > INT_MAX\n", __FILE__, __LINE__);
		exit(1);
	}

	return (int)diff;
}

void
Bol(void)
{
	curchar = 0;
}

void
Eol(void)
{
	curchar = length(curline);
}

void
Eof(void)
{
	PushPntp(curbuf->b_last);
	ToLast();
}

void
Bof(void)
{
	PushPntp(curbuf->b_first);
	ToFirst();
}

/* Move forward (if dir > 0) or backward (if dir < 0) a sentence.  Deals
 * with all the kludgery involved with paragraphs, and moving backwards
 * is particularly yucky.
 */
private void
to_sent(int dir)
{
	for (;;) {
		Bufpos
		old,	/* where we started */
		*new;	/* where dosearch stopped */
		DOTsave(&old);
		new = dosearch(
				"^[ \t]*$\\|[?.!]\\{''\\|[\"')\\]]\\|\\}\\{$\\|[ \t]\\}",
				dir, YES);

		if (new == NULL) {
			if (dir == BACKWARD) {
				ToFirst();
			} else {
				ToLast();
			}

			break;
		}

		SetDot(new);

		if (dir < 0) {
			to_word(FORWARD);

			if ((old.p_line != curline || old.p_char > curchar)
				&& (!inorder(new->p_line, new->p_char, old.p_line, old.p_char)
					|| !inorder(old.p_line, old.p_char, curline, curchar))) {
				break;
			}

			SetDot(new);
		} else {
			if (blnkp(linebuf)) {
				Bol();
				b_char(1);

				if (old.p_line != curline || old.p_char < curchar) {
					break;
				}

				to_word(FORWARD);	/* Oh brother this is painful */
			} else {
				curchar = REbom + 1;	/* Just after the [?.!] */

				if (LookingAt("''\\|[\"')\\]]", linebuf, curchar)) {
					curchar = REeom;
				}

				break;
			}
		}
	}
}

void
Bos(void)
{
	int	num = arg_value_as_int();

	if (num < 0) {
		negate_arg();
		Eos();
	} else {
		while (--num >= 0) {
			to_sent(BACKWARD);

			if (bobp()) {
				break;
			}
		}
	}
}

void
Eos(void)
{
	int	num = arg_value_as_int();

	if (num < 0) {
		negate_arg();
		Bos();
	} else {
		while (--num >= 0) {
			to_sent(FORWARD);

			if (eobp()) {
				break;
			}
		}
	}
}

void
f_word(long num)
{
	if (num < 0) {
		while (++num <= 0) {
			to_word(BACKWARD);

			while (!bolp() && jisword(linebuf[curchar - 1])) {
				curchar -= 1;
			}

			if (bobp()) {
				break;
			}
		}
	} else {
		while (--num >= 0) {
			char	c;
			to_word(FORWARD);

			while ((c = linebuf[curchar]) != '\0' && jisword(c)) {
				curchar += 1;
			}

			if (eobp()) {
				break;
			}
		}
	}

	/* ??? why is the following necessary? -- DHR */
	this_cmd = OTHER_CMD;	/* Semi kludge to stop some unfavorable behavior */
}

void
ForWord(void)
{
	f_word(arg_value());
}

void
BackWord(void)
{
	f_word(-arg_value());
}
