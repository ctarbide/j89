/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#ifdef HIGHLIGHTING

typedef void(*LEproc) ptrproto((bool));
#define LENULLPROC (LEproc)0

typedef struct LErange {
	unsigned
		start,	/* starting column for highlighting */
		width;	/* width of highlighting */
	LEproc	norm,
		high;
} *LineEffects;

#define	NOEFFECT	((LineEffects) NULL)
extern void US_effect proto((bool));

#else /* !HIGHLIGHTING */

typedef bool	LineEffects;	/* standout or not */
#define	NOEFFECT	NO

#endif /* !HIGHLIGHTING */

struct screenline {
	char
	*s_line,
	*s_roof;	/* character after last */
	LineEffects s_effects;
};

extern struct screenline
	*Screen,
	*Curline;

extern char *cursend;

extern int
AbortCnt,

CapLine,	/* cursor line and cursor column */
CapCol;

extern bool
	BufSwrite proto((int linenum)),
	swrite proto((char *line, LineEffects hl, bool abortable));

extern LineEffects
	WindowRange proto((Window *w));

extern void
	Placur proto((int line, int col)),
	cl_eol proto((void)),
	cl_scr proto((bool doit)),
	clrline proto((char *cp1, char *cp2)),
	i_set proto((int nline, int ncol)),
	make_scr proto((void)),
	v_ins_line proto((int num, int top, int bottom)),
	v_del_line proto((int num, int top, int bottom)),
	SO_effect proto((bool)),
	SO_off proto((void));

#define	TABDIST(col)	(tabstop - (col)%tabstop)	/* cols to next tabstop */

/* Variables: */

extern int	tabstop;		/* VAR: expand tabs to this number of spaces */
