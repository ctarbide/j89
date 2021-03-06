/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"
#include "jctype.h"
#include "extend.h"
#include "macros.h"
#include "mouse.h"
#include "fmt.h"

/* included for command routine declarationss */
#include "abbrev.h"
/* #include "argcount.h */
/* #include "buf.h" */
#include "c.h"
#include "case.h"
#include "commands.h"
#include "delete.h"
#include "disp.h"
/* #include "extend.h" */
#include "insert.h"
/* #include "io.h" */
#include "sysprocs.h"	/* needed for iproc.h */
#include "iproc.h"
/* #include "jove.h" */
/* #include "keymaps.h" */
/* #include "macros.h" */
#include "marks.h"
#include "misc.h"
#include "move.h"
#include "para.h"
#include "proc.h"
#include "reapp.h"
#include "wind.h"

#define	PROC(p)	(p)
#include "commands.tab"

const data_obj	*LastCmd;
const char	*ProcFmt = ": %f ";

private int
	cmdcmp_prefix proto((UnivConstPtr p1, UnivConstPtr p2)),
	cmdcmp_exact proto((UnivConstPtr p1, UnivConstPtr p2));

private int
cmdcmp_prefix(UnivConstPtr p1, UnivConstPtr p2)
{
	const struct cmd *c1 = (const struct cmd *) p1;
	const struct cmd *c2 = (const struct cmd *) p2;
	return strncmp(c1->Name, c2->Name, strlen(c1->Name));
}

private int
cmdcmp_exact(UnivConstPtr p1, UnivConstPtr p2)
{
	const struct cmd *c1 = (const struct cmd *) p1;
	const struct cmd *c2 = (const struct cmd *) p2;
	return strcmp(c1->Name, c2->Name);
}

const struct cmd *
searchcmd(char *name, int type)
{
	const struct cmd	*res;
	struct cmd	ckey;
	int (*cmdcmp)(UnivConstPtr a, UnivConstPtr b);
	ckey.Name = name;
	cmdcmp = type == SEARCHCMD_TYPE_EXACT_MATCH? cmdcmp_exact : cmdcmp_prefix;
	res = (const struct cmd *)bsearch((UnivConstPtr)&ckey,
			(UnivConstPtr)commands, elemsof(commands) - 1,/* ignore NULL */
			sizeof(struct cmd), cmdcmp);
	if (res && type == SEARCHCMD_TYPE_FIRST_PREFIX) {
		size_t len = strlen(name);
		while (res > commands && strncmp(ckey.Name, (res-1)->Name, len) == 0) {
			res--;
		}
	}
	return res;
}

const data_obj *
findcom(const char *prompt)
{
	if (InJoverc) {
		/* for faster startup, skip using the minibuffer. */
		char	cmdbuf[128];
		char	*cp = cmdbuf;
		ZXchar	c;
		const struct cmd	*which;

		/* gather the cmd name */
		while (jisprint(c = getch()) && c != ' ') {
			*cp++ = (char)CharDowncase(c);

			if (cp == &cmdbuf[sizeof(cmdbuf)]) {
				complain("command too long");
				/* NOTREACHED */
			}
		}

		*cp = '\0';

		/* TODO: random prefix is the original implementation, maybe
		 * revise this
		 */
		which = searchcmd(cmdbuf, SEARCHCMD_TYPE_RANDOM_PREFIX);

		if (which == NULL) {
			complain("[\"%s\" unknown]", cmdbuf);
			/* NOTREACHED */
		}

		return (data_obj *) which;
	} else {
		static const char	*strings[elemsof(commands)];
		static int	last = -1;

		if (strings[0] == NULL) {
			const char	**strs = strings;
			const struct cmd	*c = commands;
			do {} while ((*strs++ = (*c++).Name) != NULL);
		}

		last = complete(strings, last >= 0 ? strings[last] : (char *)NULL,
				prompt, CASEIND | ALLOW_OLD);
		return (data_obj *) &commands[last];
	}
}

const struct cmd *
FindCmd(cmdproc_t proc)
{
	const struct cmd	*cp;

	for (cp = commands; cp->Name; cp++) {
		if (cp->c_proc == proc) {
			return cp;
		}
	}

	return NULL;
}

void
ExecCmd(const data_obj *cp)
{
	LastCmd = cp;

	if (cp->Type & MAJOR_MODE) {
		SetMajor((cp->Type >> MAJOR_SHIFT));
	} else if (cp->Type & MINOR_MODE) {
		TogMinor((cp->Type >> MAJOR_SHIFT));
	} else {
		switch (cp->Type & TYPEMASK) {
			case MACRO:
				do_macro((struct macro *) cp);
				break;

			case COMMAND: {
				struct cmd	*cmd = (struct cmd *) cp;

				if (cmd->c_proc != NULL) {
					if ((cmd->Type & MODIFIER)
					    && BufMinorMode(curbuf, BReadOnly)) {
						rbell();
						message("[Buffer is read-only]");
						} else {
							(*cmd->c_proc)();
							}
					}
				}

				break;
		}
	}
}
