/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

extern void
AllMarkReset proto((Buffer *b, LinePtr line)),
	     DFixMarks proto((LinePtr line1, INTPTR_T char1, LinePtr line2, INTPTR_T char2)),
	     DelMark proto((Mark *m)),
	     IFixMarks proto((LinePtr line1, INTPTR_T char1, LinePtr line2, INTPTR_T char2)),
	     MarkSet proto((Mark *m, LinePtr line, INTPTR_T column)),
	     ToMark proto((Mark *m)),
	     flush_marks proto((Buffer *)),
	     do_set_mark proto((LinePtr l, INTPTR_T c)),
	     set_mark proto((void));

extern Mark
	*CurMark proto((void)),
	*MakeMark proto((LinePtr line, INTPTR_T column));

/* Commands: */

extern void
PopMark proto((void)),
	ExchPtMark proto((void)),
	SetMark proto((void));
