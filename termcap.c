/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"

#ifdef TERMCAP	/* the body is the rest of the file */

#include "term.h"

#include "disp.h"
#include "fmt.h"
#include "fp.h"
#include "jctype.h"
#include "screen.h"

extern int	UNMACRO(tgetent) proto((char */*buf*/, const char */*name*/));
extern int	UNMACRO(tgetflag) proto((const char */*id*/));
extern int	UNMACRO(tgetnum) proto((const char */*id*/));
extern char	*UNMACRO(tgetstr) proto((const char */*id*/, char **/*area*/));
extern void	UNMACRO(tputs) proto((const char *, int, void (*) proto((int))));

/* Termcap definitions */

const char
	*CS,	/* change scrolling region */
	*SO,	/* Start standout */
	*SE,	/* End standout */
	*US,	/* Start underlining */
	*UE,	/* End underlining */
	*CM,	/* The cursor motion string */
	*CL,	/* Clear screen */
	*CE,	/* Clear to end of line */
	*HO,	/* Home cursor */
	*AL,	/* Addline (insert line) */
	*DL,	/* Delete line */
	*VS,	/* Visual start */
	*VE,	/* Visual end */
	*KS,	/* Keypad mode start */
	*KE,	/* Keypad mode end */
	*TI,	/* Cursor addressing start */
	*TE,	/* Cursor addressing end */
	*LL,	/* Last line, first column */
	*SF = "\n",	/* Scroll forward (defaults to \n) */
	*SR,	/* Scroll reverse */
	*VB,	/* visible bell */
	*BL = "\007",	/* audible bell (defaults to BEL) */
	*lPC,	/* pad character (as a string!) */
	*NL = "\n",	/* newline character (defaults to \n) */
	*DO = "\n",	/* down one line (defaults to \n capability) */
	*M_AL,	/* Insert line with arg */
	*M_DL,	/* Delete line with arg */
	*M_SF,	/* Scroll forward with arg */
	*M_SR;	/* Scroll back with arg */

int
	UPlen = INFINITY,		/* length of the UP string */
	HOlen = INFINITY,		/* length of Home string */
	LLlen = INFINITY,		/* length of last line string */

	phystab = 8,	/* ("it") terminal's tabstop settings */
	UG;		/* number of magic cookies left by US and UE */

bool
	Hazeltine,	/* Hazeltine tilde kludge */
	UL,		/* underscores don't replace chars already on screen */
	NP;		/* there is No Pad character */

#  ifdef DEFINE_PC_BC_UP_OSPEED
	/* This is needed for HP-UX, possibly for other SYSVR2 systems */
char
	PC;		/* pad character, as a char (set from lPC; defaults to NUL) */

const char
	*BC,	/* back space (defaults to BS) */
	*UP;	/* Scroll reverse, or up */

short	ospeed;
#  endif /* DEFINE_PC_BC_UP_OSPEED */

bool	CanScroll;	/* can this terminal scroll? */

#  ifdef ID_CHAR

const char
	*IC,	/* Insert char */
	*DC,	/* Delete char */
	*IM,	/* Insert mode */
	*EI,	/* End insert mode */
	*IP,	/* insert pad after character inserted */
	*M_IC,	/* Insert char with arg */
	*M_DC;	/* Delete char with arg */

bool	UseIC = NO;	/* VAR: whether or not to use i/d char processesing */

int
#   ifdef NCURSES_BUG
	IMEIlen = INFINITY,	/* length of insert mode + end insert mode strings */
#   else
	IMEIlen = 0,	/* length of insert mode + end insert mode strings */
#   endif

	IClen = INFINITY,	/* length of insert char */
	MIClen = INFINITY,	/* length of insert char with arg */
	DClen = INFINITY,	/* length of delete char */
	MDClen = INFINITY,	/* length of delete char with arg */
	CElen = INFINITY;	/* length of clear to end of line */

bool
	MI;		/* okay to move while in insert mode */

#  endif /* ID_CHAR */

/* as of 20200104, xterm-new is 254 bytes and guru-nctxt is 289 bytes! */
private char	tspace[320];	/* space for termcap strings */

/* The ordering of ts and meas must agree !! */
private const char	ts[] =
#  ifdef ID_CHAR
/*                   |                   |                   | |             */
"vsvealdlcssoseusuecmclcehoupbcllsfsrvbksketitepcblnldoALDLSFSRicimdceiipICDC";
#  else
"vsvealdlcssoseusuecmclcehoupbcllsfsrvbksketitepcblnldoALDLSFSR";
#  endif

private const char	**const meas[] = {
	&VS, &VE, &AL, &DL, &CS, &SO, &SE, &US, &UE, &CM,
	&CL, &CE, &HO, &UP, &BC, &LL, &SF, &SR, &VB, &KS,
	&KE, &TI, &TE, &lPC, &BL, &NL, &DO, &M_AL, &M_DL, &M_SF,
	&M_SR,
#  ifdef ID_CHAR
	&IC, &IM, &DC, &EI, &IP, &M_IC, &M_DC,
#  endif
	NULL
};

struct CapLen {
	const char	**cap_var;
	int	*len_var;
};

private const struct CapLen	CapLenTab[] = {
	{ &HO,	&HOlen },
	{ &LL,	&LLlen },
	{ &UP,	&UPlen },
#  ifdef ID_CHAR
	{ &IC,	&IClen },
	{ &DC,	&DClen },
	{ &M_IC,	&MIClen },
	{ &M_DC,	&MDClen },
	{ &CE,	&CElen },
#  endif
	{ NULL,	NULL }
};

private void 
tcbad (const char *termname, const char *why)
{
	writef("You can't run JOVE on a %s terminal: %s\n", termname, why);
	flushscreen();
	_exit(1);
}

void 
getTERM (void)
{
	char	termnmbuf[13],
		*termname = getenv("TERM"),
		*termp = tspace,
		tbuff[2048];	/* Good grief! */

	if (termname == NULL || *termname == '\0'
	|| strcmp(termname, "dumb") == 0
	|| strcmp(termname, "unknown") == 0
	|| strcmp(termname, "network") == 0)
	{
		int	len;
		putstr("Enter terminal type (e.g, vt100): ");
		flushscreen();
		len = read(0, (UnivPtr) termnmbuf, sizeof(termnmbuf));
		termnmbuf[len > 1? len - 1 : 0] = '\0';
		termname = termnmbuf;
	}

	if (tgetent(tbuff, termname) < 1)
		tcbad(termname, "type unknown");

	/* get numeric capabilities */

	if ((CO = tgetnum("co")) == -1)
		tcbad(termname, "co unknown (width)");

	if (CO > MAXCOLS)
		CO = MAXCOLS;

	if ((LI = tgetnum("li")) == -1)
		tcbad(termname, "li unknown (height)");

	if ((phystab = tgetnum("it")) == -1 || phystab <= 0)
		phystab = 8;

	if ((SG = tgetnum("sg")) == -1)
		SG = 0;			/* Used for mode line only */

	if ((UG = tgetnum("ug")) == -1)
		UG = 0;			/* We shan't bother if != 0 */

	/* get string capabilities */
	{
		const char	*tsp = ts;
		const char	**const *measp;

		for (measp = meas; *measp != NULL; measp++) {
			static char	nm[3] = "xx";
			char	*val;

			nm[0] = *tsp++;
			nm[1] = *tsp++;
			val = tgetstr(nm, &termp);
			if (val != NULL)
				**measp = val;
			if (termp > tspace + sizeof(tspace))
				tcbad(termname, "too many bytes of termcap strings");
		}
	}
	if (lPC)
		PC = *lPC;	/* convert lPC string attribute to char PC */

	/* get boolean capabilities */

	Hazeltine = tgetflag("hz")==YES;	/* Hazeltine tilde kludge */
	NP = tgetflag("NP")==YES;	/* there is No Pad character */
	UL = tgetflag("ul")==YES;	/* underscores don't replace chars already on screen */

	/* adjust capabilities */

	if (tgetflag("km") == YES
#  if defined(USE_CTYPE) && !defined(NO_SETLOCALE)
	&& strcmp(LcCtype, "C") == 0
#  endif
	)
		MetaKey = YES;	/* has meta-key and default locale */

	if (tgetflag("xs") == YES) {
		SO = SE = NULL;	/* don't use braindamaged standout mode */
		SG = 0;
	}

	if (SR == NULL && M_SR == NULL)
		CS = NULL;	/* don't use scrolling region without way of reverse scrolling */

	/* Note: the way termcap/terminfo is defined, we must use *both*
	 * IC and IM to insert, but normally only one will be defined.
	 * See terminfo(5), under the heading "Insert/Delete Character".
	 * Because of this, IM might be defined as a null string.
	 *
	 * The freely redistributable termcap/terminfo database associated
	 * with ncurses breaks this rule.  This is unfortunate, but we
	 * cannot seem to get them to fix this bug.  Apparently no currently
	 * supported terminal needed this feature anyway, so we might
	 * as well go with the flow.
	 */
#  ifdef ID_CHAR
	if (IM == NULL || *IM == '\0' || EI == NULL || *EI == '\0')
		IM = EI = NULL;	/* If IM or EI is empty, supress both. */

#   ifndef NCURSES_BUG
	if (IC == NULL || *IC == '\0')
		M_IC = NULL;	/* don't know how to use this */
#   endif

	UseIC = (IC != NULL || IM != NULL || M_IC != NULL);
	MI = tgetflag("mi")==YES;	/* okay to move while in insert mode */
#  endif /* ID_CHAR */

	/* strip stupid padding information */
	while ('0' <= *NL && *NL <= '9')
		NL += 1;
	if (*NL == '*')
		NL += 1;

	if (BC == NULL)
		BC = "\b";	/* default back space to BS */

	CanScroll = (AL != NULL && DL != NULL) || CS != NULL;

	/* calculate lengths */
	{
		static const struct CapLen	*p;

		for (p = CapLenTab; p->cap_var != NULL; p++)
			if (*p->cap_var != NULL)
				*p->len_var = strlen(*p->cap_var);
#ifdef ID_CHAR
		if (IM != NULL)
			IMEIlen = strlen(IM) + strlen(EI);
#endif
	}
	if (!(CM != NULL || HO != NULL))
		tcbad(termname, "JOVE needs either cm or ho termcap/terminfo capability");
}

/* Put multi-unit or multiple single-unit strings, as appropriate. */

private void 
tputc (int c)
{
	scr_putchar(c);
}

void 
putmulti (
    const char *ss,	/* single line */
    const char *ms,	/* multiline */
    int num,	/* number of iterations */
    int lines	/* lines affected (for padding) */
)
{
	if (ms && (num > 1 || !ss)) {
		/* use the multi string */
		tputs(targ1(ms, num), lines, tputc);
	} else {
		/* repeatedly use single string */
		while (num--)
			putpad(ss, lines);
	}
}

/* put a string with padding */

void 
putpad (const char *str, int lines)
{
	if (str != NULL)
		tputs(str, lines, tputc);
}

void 
dobell (	/* declared in term.h */
    int n
)
{
	while (--n >= 0) {
		if (VisBell && VB)
			putpad(VB, 1);
		else
			putpad(BL, 1);
	}
	flushscreen();
}

void 
clr_page (void)
{
	putpad(CL, LI);
}

#endif /* TERMCAP */
