/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

struct macro {
	/* Type and Name must match data_obj */
	unsigned	Type;		/* in this case a macro */
	const char	*Name;		/* name is always second ... */
	int	m_len;		/* length of macro so we can use ^@ */
	char	*m_body;	/* actual body of the macro */
	struct macro	*m_nextm;
};

extern bool
InMacDefine;	/* are we defining a macro right now? */

extern struct macro	*macros;

extern bool
in_macro proto((void)),
	 ModMacs proto((void));

extern ZXchar
mac_getc proto((void));

extern void
mac_init proto((void)),
	 do_macro proto((struct macro *mac)),
	 unwind_macro_stack proto((void)),
	 mac_putc proto((DAPchar c)),
	 note_dispatch proto((void));

/* Commands: */
extern void
DefKBDMac proto((void)),
	  ExecMacro proto((void)),
	  Forget proto((void)),
	  MacInter proto((void)),
	  NameMac proto((void)),
	  Remember proto((void)),
	  RunMacro proto((void)),
	  WriteMacs proto((void));

/* dataobj.h:
 *	findmac
 */
