/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"
#include "disp.h"
#include "case.h"
#include "jctype.h"
#include "marks.h"
#include "move.h"

/* Commands:
 *	CapChar
 *	CapWord
 *	CasRegLower
 *	CasRegUpper
 *	LowWord
 *	UppWord
 */

private	bool
lower proto((char *)),
      upper proto((char *));

private void
CaseReg proto((bool up)),
	case_reg proto((LinePtr line1, INTPTR_T char1, LinePtr line2, INTPTR_T char2, bool up));

void
CapChar(void)
{
	long	num;
	bool	restore = NO;
	Bufpos	b;
	DOTsave(&b);
	num = arg_value();

	if (num < 0) {
		restore = YES;
		num = -num;
		b_char(num);	/* Cap previous EXP chars */
	}

	while (num--) {
		if (upper(&linebuf[curchar])) {
			modify();
			makedirty(curline);
		}

		if (eolp()) {
			if (curline->l_next == NULL) {
				break;
			}

			SetLine(curline->l_next);
		} else {
			curchar += 1;
		}
	}

	if (restore) {
		SetDot(&b);
	}
}

void
CapWord(void)
{
	long	num,
		restore = NO;
	Bufpos	b;
	DOTsave(&b);
	num = arg_value();

	if (num < 0) {
		restore = YES;
		f_word(num);		/* Cap previous EXP words */
		num = -num;
	}

	while (num--) {
		to_word(FORWARD);	/* Go to the beginning of the next word. */

		if (eobp()) {
			break;
		}

		if (upper(&linebuf[curchar])) {
			modify();
			makedirty(curline);
		}

		curchar += 1;

		while (!eolp() && jisword(linebuf[curchar])) {
			if (lower(&linebuf[curchar])) {
				modify();
				makedirty(curline);
			}

			curchar += 1;
		}
	}

	if (restore) {
		SetDot(&b);
	}
}

private void
case_word(bool up)
{
	Bufpos	before;
	DOTsave(&before);
	ForWord();	/* this'll go backward if negative argument */
	case_reg(before.p_line, before.p_char, curline, curchar, up);
}

/* Convert *p to upper case.  Return YES iff it was changed. */

private bool
upper(char *p)
{
	if (jislower(*p)) {
		*p = (char)CharUpcase(*p);
		return YES;
	}

	return NO;
}

/* Convert *p to lower case.  Return YES iff it was changed. */

private bool
lower(char *p)
{
	char c = *p;

	if (jisupper(c)) {
		*p = (char)CharDowncase(c);
		return YES;
	}

	return NO;
}

private void
case_reg(LinePtr line1, INTPTR_T char1, LinePtr line2, INTPTR_T char2, bool up)
{
	(void) fixorder(&line1, &char1, &line2, &char2);
	DotTo(line1, char1);

	for (;;) {
		if (curline == line2 && curchar == char2) {
			break;
		}

		if (!eolp())
			if ((up) ? upper(&linebuf[curchar]) : lower(&linebuf[curchar])) {
				makedirty(curline);
				modify();
			}

		f_char(1);
	}
}

void
CasRegLower(void)
{
	CaseReg(NO);
}

void
CasRegUpper(void)
{
	CaseReg(YES);
}

private void
CaseReg(bool up)
{
	Mark	*mp = CurMark();
	Bufpos	savedot;
	DOTsave(&savedot);
	case_reg(curline, curchar, mp->m_line, mp->m_char, up);
	SetDot(&savedot);
}

void
UppWord(void)
{
	case_word(YES);
}

void
LowWord(void)
{
	case_word(NO);
}
