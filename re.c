/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

/* search package */

#include "jove.h"
#include "re.h"
#include "jctype.h"
#include "ask.h"
#include "disp.h"
#include "fmt.h"
#include "marks.h"

private bool
do_comp proto((struct RE_block *, int));

char	rep_search[128],	/* replace search string */
	rep_str[128];		/* contains replacement string */

bool	CaseIgnore = NO,	/* VAR: ignore case in search */
	WrapScan = NO;		/* VAR: make searches wrap */

private ZXchar	REpeekc;
private const char	*REptr;

private ZXchar
REgetc(void)
{
	ZXchar	c;

	if ((c = REpeekc) != EOF) {
		REpeekc = EOF;
	} else if (*REptr != '\0') {
		c = ZXC(*REptr++);
	} else {
		c = EOF;
	}

	return c;
}

#define STAR	01	/* Match any number of last RE (ORed into other ops). */

#define AT_BOL	2	/* ^ */
#define AT_EOL	4	/* $ */
#define AT_BOW	6	/* \< */
#define AT_EOW	8	/* \> */
#define OPENP	10	/* \( {chunk number} */
#define CLOSEP	12	/* \) {chunk number} */
#define CURLYB	14	/* \{ {number of alt, alts } */

#define NOSTR	14	/* Codes <= NOSTR can't be *'d. */

#define ANYC	(NOSTR+2)		/* . */
#define NORMC	(ANYC+2)		/* normal chars {len, char...} */
#define CINDC	(NORMC+2)		/* case independent chars {len, char...} */
#define ONE_OF	(CINDC+2)		/* [xxx] {bitmask} */
#define NONE_OF	(ONE_OF+2)	/* [^xxx] {bitmask} */
#define BACKREF	(NONE_OF+2)	/* \# {chunk number} */
#define EOP	(BACKREF+2)	/* end of pattern */

#define CHAR_MASK	((1 << CHAR_BIT) - 1)	/* byte mask, really */
#define ALT_LEN_LEN	2	/* an alt starts with a two-byte length */
#define ALT_LEN(p)	(((p)[0] & CHAR_MASK) + (((p)[1] & CHAR_MASK) << CHAR_BIT))

/* ONE_OF/NONE_OF is represented as a bit vector.
 * These symbols parameterize the representation.
 */

#define	SETSIZE		(NCHARS / CHAR_BIT)
#define	SETBYTE(c)	((c) / CHAR_BIT)
#define	SETBIT(c)	(1 << ((c) % CHAR_BIT))

#define NPAR	10	/* [0-9] - 0th is the entire matched string, i.e. & */
private char	*comp_ptr,
		**alt_p,
		**alt_endp;

void
REcompile(const char *pattern, bool re, struct RE_block *re_blk)
{
	REptr = pattern;
	REpeekc = EOF;
	comp_ptr = re_blk->r_compbuf;
	alt_p = re_blk->r_alternates;
	alt_endp = alt_p + NALTS - 1;
	*alt_p++ = comp_ptr;
	re_blk->r_nparens = 0;
	(void) do_comp(re_blk, re ? OKAY_RE : NORM);
	*alt_p = NULL;
	re_blk->r_anchored = NO;
	re_blk->r_firstc = EOF;

	/* do a little post processing */
	if (re_blk->r_alternates[1] == NULL) {
		char	*p;
		p = re_blk->r_alternates[0];

		for (;;) {
			switch (*p) {
			case OPENP:
			case CLOSEP:
				p += 2;
				continue;

			case AT_BOW:
			case AT_EOW:
				p += 1;
				continue;

			case AT_BOL:
				re_blk->r_anchored = YES;
				/* don't set firstc -- won't work */
				break;

			case NORMC:
			case CINDC:
				re_blk->r_firstc = CharUpcase(p[2]);
				break;

			default:
				break;
			}

			break;
		}
	}
}

/* compile the pattern into an internal code */

private bool
do_comp(struct RE_block *re_blk, int kind)
{
	char	*this_verb,
		*prev_verb,
		*start_p,
		*comp_endp;
	int	parens[NPAR],
		*parenp,
		outer_max_paren = -1;
	ZXchar	c;
	bool	done_cb = NO;
	parenp = parens;
	this_verb = NULL;
	comp_endp = &re_blk->r_compbuf[COMPSIZE - 6];

	/* wrap the whole expression around (implied) parens */
	if (kind != IN_CB) {
		if (re_blk->r_nparens >= NPAR) {
			complain("Too many ('s; max is %d.", NPAR);
			/* NOTREACHED */
		}

		*comp_ptr++ = OPENP;
		/* 're_blk->r_nparens' guaranteed less than NPAR */
		*parenp++ = *comp_ptr++ = (char)re_blk->r_nparens++;
	}

	start_p = comp_ptr;

	while ((c = REgetc()) != EOF) {
		if (comp_ptr > comp_endp) {
toolong:
			complain("Search string too long/complex.");
			/* NOTREACHED */
		}

		prev_verb = this_verb;
		this_verb = comp_ptr;

		/* The following test ought to be
		 *	kind == NORM && c != '\\'
		 * but Jon likes to put ^, $, and \ in i-searches.
		 * Don't tell him, but $ only sort of works. -- DHR
		 */
		if (kind == NORM && strchr("^$\\", c) == NULL) {
			goto defchar;
		}

		switch (c) {
		case '\\':
			switch (c = REgetc()) {
			case EOF:
				complain("[Premature end of pattern]");

			/*NOTREACHED*/

			case '{': {
				char	*altcntp;		/* alternate count */
				int
				init_paren = re_blk->r_nparens,
				max_paren = -1;
				*comp_ptr++ = CURLYB;
				altcntp = comp_ptr;
				*comp_ptr++ = 0;	/* initialize alt-count */

				for (;;) {
					char	*comp_len = comp_ptr;
					bool	done;
					long	len;
					comp_ptr += ALT_LEN_LEN;
					re_blk->r_nparens = init_paren;
					done = do_comp(re_blk, IN_CB);

					/* We demand that each alternate has the same number
					 * of parens because we currently have no mechanism to
					 * set the matching strings to a meaningful default.
					 */
					if (max_paren == -1) {
						max_paren = re_blk->r_nparens;
					}

					if (max_paren != re_blk->r_nparens) {
						complain("[each alternate must have the same number of \\( \\)]");
						/* NOTREACHED */
					}

					len = comp_ptr - comp_len;
					comp_len[0] = (char) len;	/* truncate */
					comp_len[1] = (char)(len >> CHAR_BIT);	/* truncate */
					(*altcntp)++;

					if (done) {
						break;
					}
				}

				break;
			}

			case '}':
				if (kind != IN_CB) {
					complain("Unexpected \\}.");
					/* NOTREACHED */
				}

				done_cb = YES;
				goto outahere;

			case '(':
				if (re_blk->r_nparens >= NPAR) {
					complain("Too many ('s; max is %d.", NPAR);
					/* NOTREACHED */
				}

				*comp_ptr++ = OPENP;
				/* 're_blk->r_nparens' guaranteed less than NPAR */
				*parenp++ = *comp_ptr++ = (char)re_blk->r_nparens++;
				break;

			case ')':
				if (parenp == parens) {
					complain("Too many )'s.");
					/* NOTREACHED */
				}

				*comp_ptr++ = CLOSEP;
				/* parenp[-1] probably within 0 and NPAR */
				*comp_ptr++ = (char)*--parenp;
				break;

			case '|':
				if (kind == IN_CB) {
					goto outahere;
				}

				if (alt_p >= alt_endp) {
					complain("Too many alternates; max %d.", NALTS);
					/* NOTREACHED */
				}

				/* close off previous alternate */
				*comp_ptr++ = CLOSEP;
				/* parenp[-1] probably within 0 and NPAR */
				*comp_ptr++ = (char)*--parenp;

				if (parenp != parens) {
					complain("Unmatched \\(.");
					/* NOTREACHED */
				}

				*comp_ptr++ = EOP;

				/* We demand that each alternate has the same number
				 * of parens because we currently have no mechanism to
				 * set the matching strings to a meaningful default.
				 */
				if (outer_max_paren == -1) {
					outer_max_paren = re_blk->r_nparens;
				}

				if (outer_max_paren != re_blk->r_nparens) {
					complain("[each alternate must have the same number of \\( \\)]");
					/* NOTREACHED */
				}

				/* start a new alt */
				*alt_p++ = comp_ptr;
				re_blk->r_nparens = 0;
				*comp_ptr++ = OPENP;
				/* 're_blk->r_nparens' is zero */
				*parenp++ = *comp_ptr++ = (char)re_blk->r_nparens++;
				start_p = comp_ptr;
				break;

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				*comp_ptr++ = BACKREF;
				*comp_ptr++ = (char)(c - '0');
				break;

			case '<':
				*comp_ptr++ = AT_BOW;
				break;

			case '>':
				*comp_ptr++ = AT_EOW;
				break;

			default:
				goto defchar;
			}

			break;

		case '.':
			*comp_ptr++ = ANYC;
			break;

		case '^':
			if (comp_ptr == start_p) {
				*comp_ptr++ = AT_BOL;
				break;
			}

			goto defchar;

		case '$':
			if ((REpeekc = REgetc()) != EOF && REpeekc != '\\') {
				goto defchar;
			}

			*comp_ptr++ = AT_EOL;
			break;

		case '[':
			*comp_ptr++ = ONE_OF;

			if (comp_ptr + SETSIZE >= comp_endp) {
				goto toolong;
			}

			byte_zero(comp_ptr, (size_t) SETSIZE);
			c = REgetc();

			if (c == '^') {
				*this_verb = NONE_OF;
				c = REgetc();
			}

			do {
				if (c == EOF) {
					break;
				}

				if (c == '\\') {
					c = REgetc();

					if (c == EOF) {
						break;
					}
				}

				if ((REpeekc = REgetc()) == '-') {
					/* possibly a range */
					ZXchar	i = c;
					REpeekc = EOF;	/* discard '-' */
					c = REgetc();

					if (c == EOF) {
						break;
					}

					if (c == ']') {
						/* not a range after all */
						REpeekc = c;	/* push back ']' */
						c = '-';	/* recycle '-' */
						comp_ptr[SETBYTE(i)] |= (char)SETBIT(i);	/* handle initial char */
					} else {
						/* really a range: add members up to c */
						if (c == '\\') {
							c = REgetc();

							if (c == EOF) {
								break;
							}
						}

						while (i < c) {

							comp_ptr[SETBYTE(i)] |= (char)SETBIT(i);
							i += 1;
						}
					}
				}

				comp_ptr[SETBYTE(c)] |= (char)SETBIT(c);
				c = REgetc();
			} while (c != ']');

			if (c == EOF) {
				complain("Missing ].");
				/* NOTREACHED */
			}

			comp_ptr += SETSIZE;
			break;

		case '*':
			if (prev_verb == NULL || *prev_verb <= NOSTR || (*prev_verb & STAR) != 0) {
				goto defchar;
			}

			if (*prev_verb == NORMC || *prev_verb == CINDC) {
				char	lastc = comp_ptr[-1];

				/* The * operator applies only to the
				 * previous character.  Since we were
				 * building a string-matching command
				 * (NORMC or CINDC), we must split it
				 * up and work with the last character.
				 *
				 * Note that the STARed versions of these
				 * commands do not operate on strings, and
				 * so do not need or have character counts.
				 */

				if (prev_verb[1] == 1) {
					/* Only one char in string:
					 * delete old command.
					 */
					this_verb = prev_verb;
				} else {
					/* Several chars in string:
					 * strip off the last.
					 * New verb is derived from old.
					 */
					prev_verb[1]--;
					this_verb--;
					*this_verb = *prev_verb;
				}

				comp_ptr = this_verb + 1;
				*comp_ptr++ = lastc;
			} else {
				/* This command is just the previous one,
				 * whose verb we will modify.
				 */
				this_verb = prev_verb;
			}

			*this_verb |= STAR;
			break;

		default:
defchar:
			if (prev_verb == NULL
				|| !(*prev_verb == NORMC || *prev_verb == CINDC)) {
				/* create new string command */
				*comp_ptr++ = (CaseIgnore) ? CINDC : NORMC;
				*comp_ptr++ = 0;
			} else {
				/* merge this into previous string command */
				this_verb = prev_verb;
			}

			this_verb[1]++;
			*comp_ptr++ = (char)c;
			break;
		}
	}

outahere:

	/* End of pattern, let's do some error checking. */
	if (kind != IN_CB) {
		*comp_ptr++ = CLOSEP;
		/* parenp[-1] probably within 0 and NPAR */
		*comp_ptr++ = (char)*--parenp;
	}

	if (parenp != parens) {
		complain("Unmatched \\(.");
		/* NOTREACHED */
	}

	if (kind == IN_CB && c == EOF)	{ /* end of pattern with missing \}. */
		complain("Missing \\}.");
		/* NOTREACHED */
	}

	*comp_ptr++ = EOP;

	/* We demand that each alternate has the same number
	 * of parens because we currently have no mechanism to
	 * set the matching strings to a meaningful default.
	 */
	if (outer_max_paren != -1 && outer_max_paren != re_blk->r_nparens) {
		complain("[each alternate must have the same number of \\( \\)]");
		/* NOTREACHED */
	}

	return done_cb;
}

private char	*pstrtlst[NPAR],	/* index into re_blk->r_lbuf */
		*pendlst[NPAR],
		*REbolp,	/* begining-of-line pointer */
		*locrater,	/* roof of last substitution */
		*loc1,		/* start of matched text */
		*loc2;		/* roof of matched text */

int	REbom,		/* beginning and end columns of match */
	REeom,
	REdelta;	/* increase in line length due to last re_dosub */

private bool
backref(int n, register char *linep)
{
	register char	*backsp,
		   *backep;
	backsp = pstrtlst[n];
	backep = pendlst[n];

	while (*backsp++ == *linep++)
		if (backsp >= backep) {
			return YES;
		}

	return NO;
}

private bool
member(char *a_comp_ptr, ZXchar c, bool af)
{
	return c != '\0' && ((a_comp_ptr[SETBYTE(c)] & SETBIT(c)) ? af : !af);
}

private bool
REmatch(char *linep, char *a_comp_ptr)
{
	char	*first_p;
	register int	n;

	for (;;) switch (*a_comp_ptr++) {
		case NORMC:
			n = *a_comp_ptr++;

			while (--n >= 0)
				if (*linep++ != *a_comp_ptr++) {
					return NO;
				}

			continue;

		case CINDC:	/* case independent comparison */
			n = *a_comp_ptr++;

			while (--n >= 0)
				if (!cind_eq(*linep++, *a_comp_ptr++)) {
					return NO;
				}

			continue;

		case EOP: {
			INTPTR_T diff = loc2 - REbolp;
			if ((UINTPTR_T)diff > INT_MAX) {
				fprintf(stderr, "fatal: %s:%d: diff > INT_MAX, diff: %li\n", __FILE__, __LINE__, diff);
				exit(1);
			}

			loc2 = linep;
			/* diff guaranteed within 0 and INT_MAX */
			REeom = (int)diff;
			return YES;	/* Success! */
		}

		case AT_BOL:
			if (linep == REbolp && linep != locrater) {
				continue;
			}

			return NO;

		case AT_EOL:
			if (*linep == '\0') {
				continue;
			}

			return NO;

		case ANYC:
			if (*linep++ != '\0') {
				continue;
			}

			return NO;

		case AT_BOW:
			if (linep != locrater && jisident(*linep)
				&& (linep == REbolp || !jisident(linep[-1]))) {
				continue;
			}

			return NO;

		case AT_EOW:
			if (linep != locrater && (*linep == '\0' || !jisident(*linep))
				&& (linep != REbolp && jisident(linep[-1]))) {
				continue;
			}

			return NO;

		case ONE_OF:
		case NONE_OF:
			if (member(a_comp_ptr, ZXC(*linep++), a_comp_ptr[-1] == ONE_OF)) {
				a_comp_ptr += SETSIZE;
				continue;
			}

			return NO;

		case OPENP:
			pstrtlst[(int) *a_comp_ptr++] = linep;
			continue;

		case CLOSEP:
			pendlst[(int) *a_comp_ptr++] = linep;
			continue;

		case BACKREF:
			if (pstrtlst[n = *a_comp_ptr++] == NULL) {
				s_mess("\\%d was not specified.", n + 1);
			} else if (backref(n, linep)) {
				linep += pendlst[n] - pstrtlst[n];
				continue;
			}

			return NO;

		case CURLYB: {
			int	altcnt = *a_comp_ptr++;
			bool	any = NO;

			while (--altcnt >= 0) {
				if (!any) {
					any = REmatch(linep, a_comp_ptr + ALT_LEN_LEN);
				}

				a_comp_ptr += ALT_LEN(a_comp_ptr);
			}

			if (!any) {
				return NO;
			}

			linep = loc2;
			continue;
		}

		case ANYC | STAR:
			first_p = linep;
			do {} while (*linep++ != '\0');

			goto star;

		case NORMC | STAR:
			first_p = linep;
			do {} while (*a_comp_ptr == *linep++);

			a_comp_ptr += 1;
			goto star;

		case CINDC | STAR:
			first_p = linep;
			do {} while (cind_eq(*a_comp_ptr, *linep++));

			a_comp_ptr += 1;
			goto star;

		case ONE_OF | STAR:
		case NONE_OF | STAR:
			first_p = linep;
			do {} while (member(a_comp_ptr, ZXC(*linep++), a_comp_ptr[-1] == (ONE_OF | STAR)));

			a_comp_ptr += SETSIZE;
			/* fall through */
star:

			/* linep points *after* first unmatched char.
			 * first_p points at where starred element started matching.
			 */
			while (--linep > first_p) {
				if ((*a_comp_ptr != NORMC || *linep == a_comp_ptr[2])
					&& REmatch(linep, a_comp_ptr)) {
					return YES;
				}
			}

			continue;

		case BACKREF | STAR:
			first_p = linep;
			n = *a_comp_ptr++;

			while (backref(n, linep)) {
				linep += pendlst[n] - pstrtlst[n];
			}

			while (linep > first_p) {
				if (REmatch(linep, a_comp_ptr)) {
					return YES;
				}

				linep -= pendlst[n] - pstrtlst[n];
			}

			continue;

		default:
			complain("RE error match (%d).", a_comp_ptr[-1]);
			/* NOTREACHED */
		}

	/* NOTREACHED */
}

private void
REreset(void)
{
	register int	i;

	for (i = 0; i < NPAR; i++) {
		pstrtlst[i] = pendlst[i] = NULL;
	}
}

/* Index LINE at OFFSET.  If lbuf_okay is YES it's okay to use linebuf
 * if LINE is the current line.  This should save lots of time in things
 * like paren matching in LISP mode.  Saves all that copying from linebuf
 * to a local buffer.  substitute() is the guy who calls re_lindex with
 * lbuf_okay as NO, since the substitution gets placed in linebuf ...
 * doesn't work too well when the source and destination strings are the
 * same.  I hate all these arguments!
 *
 * This code is cumbersome, repetetive for reasons of efficiency.  Fast
 * search is a must as far as I am concerned.
 */

bool
re_lindex(
	LinePtr line,
	int offset,
	int dir,
	struct RE_block *re_blk,
	bool lbuf_okay,
	int crater	/* offset of previous substitute (or -1) */
)
{
	char	*p;
	ZXchar	firstc = re_blk->r_firstc;
	bool	anchored = re_blk->r_anchored;
	char	**alts = re_blk->r_alternates;
	REreset();

	if (lbuf_okay) {
		REbolp = lbptr(line);

		if (offset == -1) {
			offset = (int)strlen(REbolp);        /* arg! */
		}
	} else {
		REbolp = ltobuf(line, re_blk->r_lbuf);

		if (offset == -1) {	/* Reverse search, find end of line. */
			offset = Jr_Len;	/* Just Read Len. */
		}
	}

	if (anchored) {
		if (dir == FORWARD) {
			if (offset != 0 || crater != -1) {
				return NO;
			}
		} else {
			offset = 0;
		}
	}

	p = REbolp + offset;
	locrater = REbolp + crater;

	if (firstc != EOF) {
		char	*first_alt = *alts;

		if (dir == FORWARD) {
			while (CharUpcase(*p) != firstc || !REmatch(p, first_alt))
				if (*p++ == '\0') {
					return NO;
				}
		} else {
			while (CharUpcase(*p) != firstc || !REmatch(p, first_alt))
				if (--p < REbolp) {
					return NO;
				}
		}
	} else {
		for (;;) {
			char	**altp = alts;

			while (*altp != NULL)
				if (REmatch(p, *altp++)) {
					goto success;
				}

			if (anchored
				|| (dir == FORWARD ? *p++ == '\0' : --p < REbolp)) {
				return NO;
			}
		}

success:
		;
	}

	loc1 = p;
	{
		INTPTR_T diff = loc1 - REbolp;

		if ((UINTPTR_T)diff > INT_MAX) {
			fprintf(stderr, "fatal: %s:%d: diff > INT_MAX\n", __FILE__, __LINE__);
			exit(1);
		}

		/* diff guaranteed within 0 and INT_MAX */
		REbom = (int)diff;
	}
	return YES;
}

bool	okay_wrap = NO;	/* Do a wrap search ... not when we're
			   parsing errors ... */

Bufpos *
dosearch(const char *pattern, int dir, bool re)
{
	Bufpos	*pos;
	struct RE_block	re_blk;		/* global re-compiled buffer */

	if (bobp() && eobp()) {	/* Can't match!  There's no buffer. */
		return NULL;
	}

	REcompile(pattern, re, &re_blk);
	pos = docompiled(dir, &re_blk);
	return pos;
}

Bufpos *
docompiled(int dir, register struct RE_block *re_blk)
{
	static Bufpos	ret;
	register LinePtr	lp;
	register int	offset;
	bool	we_wrapped = NO;
	lsave();
	/* Search now lsave()'s so it doesn't make any assumptions on
	 * whether the the contents of curline/curchar are in linebuf.
	 * Nowhere does search write all over linebuf.  However, we have to
	 * be careful about what calls we make here, because many of them
	 * assume (and rightly so) that curline is in linebuf.
	 */
	lp = curline;
	offset = curchar;

	if (dir == BACKWARD) {
		if (bobp()) {
			if (okay_wrap && WrapScan) {
				goto doit;
			}

			return NULL;
		}

		/* here we simulate BackChar() */
		if (bolp()) {
			lp = lp->l_prev;
			offset = length(lp);
		} else {
			offset -= 1;
		}
	} else if (dir == FORWARD && lbptr(lp)[offset] == '\0' && !lastp(lp)) {
		lp = lp->l_next;
		offset = 0;
	}

	do {
		if (re_lindex(lp, offset, dir, re_blk, YES, -1)) {
			break;
		}

doit:
		lp = (dir == FORWARD) ? lp->l_next : lp->l_prev;

		if (lp == NULL) {
			if (okay_wrap && WrapScan) {
				lp = (dir == FORWARD)
					? curbuf->b_first : curbuf->b_last;
				we_wrapped = YES;
			} else {
				break;
			}
		}

		if (dir == FORWARD) {
			offset = 0;
		} else {
			offset = -1;        /* signals re_lindex ... */
		}
	} while (lp != curline);

	if (lp == curline && we_wrapped) {
		lp = NULL;
	}

	if (lp == NULL) {
		return NULL;
	}

	ret.p_line = lp;
	ret.p_char = (dir == FORWARD) ? REeom : REbom;
	return &ret;
}

private char *
insert(char *off, char *endp, int which)
{
	char	*pp = pstrtlst[which];
	int	n;

	if (pp == NULL) {
		complain("\\%d not defined", which);
		/* NOTREACHED */
	}

	{
		INTPTR_T diff = pendlst[which] - pp;

		if ((UINTPTR_T)diff > INT_MAX) {
			fprintf(stderr, "fatal: %s:%d: diff > INT_MAX\n", __FILE__, __LINE__);
			exit(1);
		}

		/* diff guaranteed within 0 and INT_MAX */
		n  = (int)diff;
	}

	/* note: ensure space will be left for a NUL */
	if (off + n >= endp) {
		len_error(JMP_ERROR);
		/* NOTREACHED */
	}

	while (--n >= 0) {
		*off++ = *pp++;
	}

	return off;
}

/* Perform the substitution.  If DELP is YES the matched string is
 * deleted, i.e., the substitution string is not inserted.
 */
void
re_dosub(struct RE_block *re_blk, char *tobuf, bool delp)
{
	register char	*tp,
		   *rp;
	char	*endp;
	tp = tobuf;
	endp = tp + LBSIZE;
	rp = re_blk->r_lbuf;

	while (rp < loc1) {
		*tp++ = *rp++;
	}

	if (!delp) {
		register char	c;
		rp = rep_str;

		while ((c = *rp++) != '\0') {
			if (c == '\\') {
				c = *rp++;

				if (c >= '0' && c < re_blk->r_nparens + '0') {
					tp = insert(tp, endp, c - '0');
					continue;
				}

				if (c == '\0') {
					/* treat \ at the end as if it were \\ */
					c = '\\';
					rp--;	/* be sure to hit again */
				}
			}

			*tp++ = c;

			if (tp >= endp) {
				len_error(JMP_ERROR);
				/* NOTREACHED */
			}
		}
	}

	rp = loc2;
	REdelta = -REeom;

	{
		INTPTR_T diff = tp - tobuf;

		if ((UINTPTR_T)diff > INT_MAX) {
			fprintf(stderr, "fatal: %s:%d: diff > INT_MAX\n", __FILE__, __LINE__);
			exit(1);
		}

		/* diff guaranteed within 0 and INT_MAX */
		REeom = (int)diff;
	}

	REdelta += REeom;

	if (loc1 == rp && *rp != '\0') {
		/* Skip an extra character if the matched text was a null
		 * string, but don't skip over the end of line.  This is to
		 * prevent an infinite number of replacements in the same
		 * position, e.g., replace "^" with "".
		 */
		REeom += 1;
	}

	loc2 = re_blk->r_lbuf + REeom;

	while ((*tp++ = *rp++) != '\0') {
		if (tp >= endp) {
			len_error(JMP_ERROR);
			/* NOTREACHED */
		}
	}
}

void
putmatch(which, buf, size)
int which;
char	*buf;
size_t size;
{
	*(insert(buf, buf + size, which)) = '\0';
}

void
RErecur(void)
{
	char	repbuf[sizeof rep_str];
	Mark	*m = MakeMark(curline, REbom);
	message("Type ^X ^C to continue with query replace.");
	byte_copy(rep_str, repbuf, sizeof rep_str);
	Recur();
	byte_copy(repbuf, rep_str, sizeof rep_str);

	if (!is_an_arg()) {
		ToMark(m);
	}

	DelMark(m);
}

/* Do we match PATTERN at OFFSET in BUF? */

bool
LookingAt(const char *pattern, char *buf, int offset)
{
	struct RE_block	re_blk;
	char	**alt = re_blk.r_alternates;
	REcompile(pattern, YES, &re_blk);
	REreset();
	locrater = NULL;
	REbolp = buf;

	while (*alt)
		if (REmatch(buf + offset, *alt++)) {
			return YES;
		}

	return NO;
}

bool
look_at(char *expr)
{
	struct RE_block	re_blk;
	REcompile(expr, NO, &re_blk);
	REreset();
	locrater = NULL;
	REbolp = linebuf;
	return REmatch(linebuf + curchar, re_blk.r_alternates[0]);
}
