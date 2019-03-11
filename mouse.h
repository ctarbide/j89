/************************************************************************
 * This program is Copyright (C) 1986-1994 by Jonathan Payne.  JOVE is  *
 * provided to you without charge, and with no warranty.  You may give  *
 * away copies of JOVE, including sources, provided that this notice is *
 * included in all the files.                                           *
 ************************************************************************/

#ifdef MOUSE	/* the body is the rest of this file */

extern void
	MouseOn proto((void)),
	MouseOff proto((void)),

	xjMousePoint proto((void)),
	xjMouseMark proto((void)),
	xjMouseWord proto((void)),
	xjMouseLine proto((void)),
	xjMouseYank proto((void)),
	xjMouseCopyCut proto((void)),

	xtMousePoint proto((void)),
	xtMouseMark proto((void)),
	xtMouseUp proto((void));

extern bool	XtermMouse;	/* VAR: should we enable xterm mouse? */

#endif /* MOUSE */
