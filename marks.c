/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"
#include "fmt.h"
#include "marks.h"
#include "disp.h"

private Mark	*FreeMarksList = NULL;

#define RecycleMark(m)	{ \
	(m)->m_next = FreeMarksList; \
	FreeMarksList = (m); \
	(m)->m_line = NULL; /* DEBUG */ \
}

Mark *
MakeMark(register LinePtr line, INTPTR_T column)
{
	register Mark	*newmark;

	if ((newmark = FreeMarksList) != NULL) {
		FreeMarksList = newmark->m_next;
	} else {
		newmark = (Mark *) emalloc(sizeof * newmark);
	}

	MarkSet(newmark, line, column);
	newmark->m_big_delete = NO;
	newmark->m_next = curbuf->b_marks;
	curbuf->b_marks = newmark;
	return newmark;
}

void
flush_marks(Buffer *b)
{
	register Mark	*m,
		   *next;

	for (m = b->b_marks; m != NULL; m = next) {
		next = m->m_next;
		RecycleMark(m)
	}
}

private void
DelBufMark(Buffer *b, register Mark *m)
{
	register Mark	*mp = b->b_marks;

	if (MarkHighlighting) {
		makedirty(m->m_line);
	}

	if (m == mp) {
		b->b_marks = m->m_next;
	} else {
		while (mp != NULL && mp->m_next != m) {
			mp = mp->m_next;
		}

		if (mp == NULL) {
			complain("Unknown mark!");
			/* NOTREACHED */
		}

		mp->m_next = m->m_next;
	}

	RecycleMark(m);
}

void
DelMark(register Mark *m)
{
	DelBufMark(curbuf, m);
}

/* AllMarkReset is used when a buffer is completely replaced.
 * We delete the marks in the mark ring, but we cannot find
 * the references to other marks, so those we make point
 * to the start of the buffer.
 */

void
AllMarkReset(Buffer *b, register LinePtr line)
{
	{
		register Mark	**mpp;

		for (mpp = &b->b_markring[0]; mpp != &b->b_markring[NMARKS]; mpp++) {
			if (*mpp != NULL) {
				DelBufMark(b, *mpp);
				*mpp = NULL;
			}
		}

		b->b_themark = 0;
	}
	{
		register Mark	*mp;

		for (mp = b->b_marks; mp != NULL; mp = mp->m_next) {
			MarkSet(mp, line, 0);
		}
	}
}

void
MarkSet(Mark *m, LinePtr line, INTPTR_T column)
{
	m->m_line = line;
	m->m_char = column;
	m->m_big_delete = NO;

	if (MarkHighlighting) {
		makedirty(line);
	}
}

void
PopMark(void)
{
	int	pmark;

	if (curmark == NULL) {
		return;
	}

	ExchPtMark();
	pmark = curbuf->b_themark;

	do {
		if (--pmark < 0) {
			pmark = NMARKS - 1;
		}
	} while (curbuf->b_markring[pmark] == NULL);

	curbuf->b_themark = pmark;

	if (MarkHighlighting) {
		makedirty(curmark->m_line);
	}
}

void
SetMark(void)
{
	if (is_an_arg()) {
		PopMark();
	} else {
		set_mark();
	}
}

void
set_mark(void)
{
	do_set_mark(curline, curchar);
}

void
do_set_mark(LinePtr l, INTPTR_T c)
{
	Mark	**mr = curbuf->b_markring;
	int	tm = curbuf->b_themark;

	if (mr[tm] != NULL) {
		if (MarkHighlighting) {
			makedirty(mr[tm]->m_line);
		}

		curbuf->b_themark = tm = (tm + 1) % NMARKS;

		if (mr[NMARKS - 1] == NULL) {
			/* there is an empty slot: make sure one is here */
			int	i;

			for (i = NMARKS - 1; i != tm; i--) {
				mr[i] = mr[i - 1];
			}

			mr[i] = NULL;
		}
	}

	if (mr[tm] == NULL) {
		mr[tm] = MakeMark(l, c);
	} else {
		MarkSet(mr[tm], l, c);
	}

	s_mess("[Point pushed]");
}

/* Move point to Mark */

void
ToMark(Mark *m)
{
	int	len;

	if (m == NULL) {
		return;
	}

	DotTo(m->m_line, m->m_char);

	if (curchar > (len = length(curline))) {
		curchar = len;
	}
}

Mark *
CurMark(void)
{
	if (curmark == NULL) {
		complain("No mark.");
		/* NOTREACHED */
	}

	return curmark;
}

void
ExchPtMark(void)
{
	LinePtr	mline;
	INTPTR_T
		mchar;
	Mark	*m = CurMark();
	mline = curline;
	mchar = curchar;
	ToMark(m);

	if (MarkHighlighting) {
		makedirty(m->m_line);
	}

	MarkSet(m, mline, mchar);
}

/* Fix marks after a deletion. */

void
DFixMarks(register LinePtr line1, INTPTR_T char1, register LinePtr line2, INTPTR_T char2)
{
	register Mark	*m;
	LinePtr	lp;

	if (curbuf->b_marks == NULL) {
		return;
	}

	for (lp = line1; lp != line2->l_next; lp = lp->l_next) {
		for (m = curbuf->b_marks; m != NULL; m = m->m_next) {
			if (m->m_line == lp
				&& (lp != line1 || m->m_char > char1)) {
				if (lp == line2 && m->m_char > char2) {
					m->m_char -= char2 - char1;
				} else {
					m->m_char = char1;

					if (line1 != line2) {
						m->m_big_delete = YES;
					}
				}

				m->m_line = line1;
			}
		}
	}
}

/* Fix marks after an insertion. */

void
IFixMarks(register LinePtr line1, INTPTR_T char1, register LinePtr line2, INTPTR_T char2)
{
	register Mark	*m;

	for (m = curbuf->b_marks; m != NULL; m = m->m_next) {
		if (m->m_line == line1 && m->m_char > char1) {
			m->m_line = line2;
			m->m_char += char2 - char1;
		}
	}
}
