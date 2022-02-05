/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

/* Routines to perform all kinds of deletion.  */

#include "jove.h"
#include "jctype.h"
#include "disp.h"
#include "delete.h"
#include "insert.h"
#include "marks.h"
#include "move.h"

/* Assumes that either line1 or line2 is actually the current line, so it can
 * put its result into linebuf.
 */
private void
patchup(LinePtr line1, register int char1, LinePtr line2, register int char2)
{
	if (line1 != line2) {
		ChkWindows(line1, line2);
	}

	DotTo(line1, char1);
	modify();
	linecopy(linebuf, curchar, lcontents(line2) + char2);

	/* The following is a redisplay optimization. */
	if (line1 != line2 && (char1 == 0 && char2 == 0)) {
		line1->l_dline = line2->l_dline;
	}

	DFixMarks(line1, char1, line2, char2);
	makedirty(curline);
}

/* Deletes the region by unlinking the lines in the middle,
 * and patching things up.  The unlinked lines are still in
 * order.
 */
LinePtr
reg_delete(LinePtr line1, int char1, LinePtr line2, int char2)
{
	register LinePtr	retline;

	if ((line1 == line2 && char1 == char2) || line2 == NULL) {
		complain((char *)NULL);
		/* NOTREACHED */
	}

	(void) fixorder(&line1, &char1, &line2, &char2);
	retline = nbufline();	/* New buffer line */
	(void) ltobuf(line1, genbuf);

	if (line1 == line2) {
		genbuf[char2] = '\0';
	}

	retline->l_prev = NULL;
	retline->l_dline = jputline(&genbuf[char1]);
	patchup(line1, char1, line2, char2);

	if (line1 == line2) {
		retline->l_next = NULL;
	} else {
		retline->l_next = line1->l_next;
		(void) ltobuf(line2, genbuf);
		genbuf[char2] = '\0';
		line2->l_dline = jputline(genbuf);
		/* Shorten this line */
	}

	if (line1 != line2) {
		line1->l_next = line2->l_next;

		if (line1->l_next) {
			line1->l_next->l_prev = line1;
		} else {
			curbuf->b_last = line1;
		}

		line2->l_next = NULL;
	}

	return retline;
}

private void
lremove(register LinePtr line1, register LinePtr line2)
{
	LinePtr	next = line1->l_next;

	if (line1 == line2) {
		return;
	}

	line1->l_next = line2->l_next;

	if (line1->l_next) {
		line1->l_next->l_prev = line1;
	} else {
		curbuf->b_last = line1;
	}

	lfreereg(next, line2);	/* Put region at end of free line list. */
}

/* delete character forward */

void
DelNChar(void)
{
	del_char(FORWARD, arg_value_as_int(), YES);
}

/* Delete character backward */

void
DelPChar(void)
{
	if (MinorMode(OverWrite) && !eolp()) {
		/* Overwrite with spaces.
		 * Some care is exercised to overwrite tabs reasonably,
		 * but control characters displayed as two are not handled.
		 */
		int	rightcol = calc_pos(linebuf, curchar);
		int	charcount = jmin(arg_value_as_int(), curchar);
		int	colcount = rightcol - calc_pos(linebuf, curchar - charcount);
		b_char(charcount);
		overwrite(' ', colcount);
		b_char(colcount);
	} else {
		del_char(BACKWARD, arg_value_as_int(), YES);
	}
}

/* Delete some characters.  If deleting forward then call for_char
 * to the final position otherwise call back_char.  Then delete the
 * region between the two with patchup().
 */
void
del_char(int dir, int num, bool OK_kill)
{
	Bufpos	before,
		after;
	bool	killp = (OK_kill && (abs(num) > 1));
	DOTsave(&before);

	if (dir == FORWARD) {
		f_char(num);
	} else {
		b_char(num);
	}

	if (before.p_line == curline && before.p_char == curchar) {
		complain((char *)NULL);
		/* NOTREACHED */
	}

	if (killp) {
		reg_kill(before.p_line, before.p_char, YES);
	} else {
		DOTsave(&after);
		(void) fixorder(&before.p_line, &before.p_char, &after.p_line, &after.p_char);
		patchup(before.p_line, before.p_char, after.p_line, after.p_char);
		lremove(before.p_line, after.p_line);
	}
}

/* The kill ring.
 * Newest entry is at killptr; second newest is at killptr-1, etc.
 * All empty slots are at the end of the array.
 */

LinePtr	killbuf[NUMKILLS];
int	killptr = 0;	/* index of newest entry (if any) */

void
DelKillRing(void)	/* delete newest entry */
{
	int	i;
	lfreelist(killbuf[killptr]);	/* free entry */

	/* move space to end */
	for (i = killptr; i != NUMKILLS - 1; i++) {
		killbuf[i] = killbuf[i + 1];
	}

	killbuf[i] = NULL;
	/* make killptr index predecessor (if any) */
	killptr = (killptr + NUMKILLS - 1) % NUMKILLS;

	while (killbuf[killptr] == NULL && killptr != 0) {
		killptr -= 1;
	}
}

private void
AddKillRing(  /* add a new entry */
	LinePtr text
)
{
	if (killbuf[killptr] != NULL) {
		killptr = (killptr + 1) % NUMKILLS;

		if (killbuf[NUMKILLS - 1] == NULL) {
			/* there is space: move one slot here */
			int	i;

			for (i = NUMKILLS - 1; i != killptr; i--) {
				killbuf[i] = killbuf[i - 1];
			}
		} else {
			/* no free slots: delete oldest element */
			lfreelist(killbuf[killptr]);
		}
	}

	killbuf[killptr] = text;
}

/* This kills a region between point, and line1/char1 and puts it on
 * the kill-ring.  If the last command was one of the kill commands,
 * the region is appended (prepended if backwards) to the last entry.
 */
void
reg_kill(LinePtr line2, int char2, bool dot_moved)
{
	LinePtr	nl,
		line1 = curline;
	int	char1 = curchar;
	bool	backwards;
	backwards = !fixorder(&line1, &char1, &line2, &char2);

	/* This is a kludge!  But it possible for commands that don't
	 * know which direction they are deleting in (e.g., delete
	 * previous word could have been called with a negative argument
	 * in which case, it wouldn't know that it really deleted
	 * forward.
	 */
	if (!dot_moved) {
		backwards = !backwards;
	}

	DotTo(line1, char1);
	nl = reg_delete(line1, char1, line2, char2);

	if (last_cmd != KILLCMD) {
		AddKillRing(nl);
	} else {
		LinePtr	lastln = lastline(nl);

		if (backwards) {
			(void) DoYank(nl, 0, lastln, length(lastln), killbuf[killptr], 0, (Buffer *)NULL);
		} else {
			LinePtr	olastln = lastline(killbuf[killptr]);
			(void) DoYank(nl, 0, lastln, length(lastln), olastln, length(olastln), (Buffer *)NULL);
		}
	}

	this_cmd = KILLCMD;
}

void
DelReg(void)
{
	register Mark	*mp = CurMark();
	reg_kill(mp->m_line, mp->m_char, NO);
}

/* get a new line buffer and add it to the kill ring */
LinePtr
new_kill(void)
{
	register LinePtr	nl = nbufline();
	AddKillRing(nl);
	SavLine(nl, NullStr);
	nl->l_next = nl->l_prev = NULL;
	return nl;
}

/* Save a region.  A pretend kill. */

void
CopyRegion(void)
{
	LinePtr	nl;
	Mark	*mp;
	long	status;
	mp = CurMark();

	if (mp->m_line == curline && mp->m_char == curchar) {
		complain((char *)NULL);
		/* NOTREACHED */
	}

	nl = new_kill();
	status = inorder(mp->m_line, mp->m_char, curline, curchar);

	if (status == -1) {
		return;
	}

	if (status) {
		DoYank(mp->m_line, mp->m_char, curline, curchar,
			nl, 0, (Buffer *)NULL);
	} else {
		DoYank(curline, curchar, mp->m_line, mp->m_char,
			nl, 0, (Buffer *)NULL);
	}
}

void
DelWtSpace(void)
{
	char	*ep = &linebuf[curchar],
		 *sp = &linebuf[curchar];

	while (jiswhite(*ep)) {
		ep += 1;
	}

	while (sp > linebuf && jiswhite(sp[-1])) {
		sp -= 1;
	}

	if (sp != ep) {
		INTPTR_T diff = sp - linebuf;

		if ((UINTPTR_T)diff > INT_MAX) {
			fprintf(stderr, "fatal: %s:%d: diff > INT_MAX\n", __FILE__, __LINE__);
			exit(1);
		}

		curchar = (int) diff;
		DFixMarks(curline, curchar, curline, curchar + (int)(ep - sp));

		/* Shift the remaining characters left in the buffer to close the gap.
		 * strcpy(sp, ep) won't do because the destination overlaps the source.
		 */
		do {} while ((*sp++ = *ep++) != '\0');

		makedirty(curline);
		modify();
	}
}

void
DelBlnkLines(void)
{
	register Mark	*dot;
	bool	all;

	if (!blnkp(&linebuf[curchar])) {
		return;
	}

	dot = MakeMark(curline, curchar);
	all = !blnkp(linebuf);

	while (blnkp(linebuf) && curline->l_prev) {
		SetLine(curline->l_prev);
	}

	all |= firstp(curline);
	Eol();
	DelWtSpace();
	line_move(FORWARD, 1, NO);

	while (blnkp(linebuf) && !eobp()) {
		DelWtSpace();
		del_char(FORWARD, 1, NO);
	}

	if (!all && !eobp()) {
		open_lines(1);
	}

	ToMark(dot);
	DelMark(dot);
}

private void
dword(bool forward)
{
	Bufpos	savedot;
	DOTsave(&savedot);

	if (forward) {
		ForWord();
	} else {
		BackWord();
	}

	reg_kill(savedot.p_line, savedot.p_char, YES);
}

void
DelNWord(void)
{
	dword(YES);
}

void
DelPWord(void)
{
	dword(NO);
}
