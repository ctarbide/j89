__END_DECLS private void 
get_hdir (register char *user, register char *buf)
{
	struct passwd	*p;

	p = getpwnam(user);
	endpwent();
	if (p == NULL) {
		add_mess(" [unknown user: %s]", user);
		SitFor(7);
		complain((char *)NULL);
		/* NOTREACHED */
	}
	jamstrsub(buf, p->pw_dir, (size_t)FILESIZE);
}

# else /* ! USE_GETPWNAM */

#  include "re.h"

private void 
get_hdir (register char *user, register char *buf)
{
	char	fbuf[LBSIZE],
		pattern[100];
	register int	u_len;
	File	*fp;

	u_len = strlen(user);
	fp = open_file("/etc/passwd", fbuf, F_READ, YES);
	swritef(pattern, sizeof(pattern),
		"%s:[^:]*:[^:]*:[^:]*:[^:]*:\\([^:]*\\):", user);
	while (!f_gets(fp, genbuf, LBSIZE))
		if ((strncmp(genbuf, user, u_len) == 0)
		&& LookingAt(pattern, genbuf, 0)) {
			putmatch(1, buf, FILESIZE);
			close_file(fp);
			return;
		}
	close_file(fp);
	add_mess(" [unknown user: %s]", user);
	SitFor(7);
	complain((char *)NULL);
	/* NOTREACHED */
}

# endif /* USE_GETPWNAM */
#endif /* UNIX */

/* Concatenate two parts of a pathname.
 * They should end up separated by a '/' (or optionally '\\', if MSFILESYSTEM)
 * but we are careful not to add one if the prefix already ends in one.
 * Arbitrarily, if pre is empty, the result is the same as if it were root (/).
 */
void
PathCat(buf, buflen, pre, post)
char	*buf;
size_t	buflen;
const char	*pre, *post;
{
	size_t	prelen = strlen(pre);

	swritef(buf, buflen,
		prelen > 0
			&& (pre[prelen-1] == '/'
#ifdef	MSFILESYSTEM
				|| pre[prelen-1] == '\\'
#endif
			   )? "%s%s" : "%s/%s",
		pre, post);
}

/* Convert path in name into a more-canonical one in intobuf.
 * - makes path absolute
 * - handles ~ (and \~, if not MSFILESYSTEM)
 * - if MSFILESYSTEM, turns \ into /
 * - on MSDOS, lower cases everything
 * Note: name and intobuf must not be aliases.
 * Note: because \~ is turned into ~, this routine is not idempotent.
 * ??? I suspect that there are places where in the code that presume
 * it is idempotent!  DHR
 */
void 
PathParse (const char *name, char *intobuf)
{
	char	localbuf[FILESIZE];

	intobuf[0] = localbuf[0] = '\0';
	if (*name == '\0')
		return;

	/* Place pathname in localbuf, with any specified home directory */

	if (*name == '~') {
		if (name[1] == '/' || name[1] == '\0') {
			strcpy(localbuf, HomeDir);
			name += 1;
		}
#ifdef UNIX	/* may add for mac in future */
		else {
			const char	*uendp = strchr(name, '/');
			char	unamebuf[30];

			if (uendp == NULL)
				uendp = name + strlen(name);
			name += 1;
			if ((size_t) (uendp - name) >= sizeof(unamebuf)) {
				len_error(JMP_COMPLAIN);
				/* NOTREACHED */
			}
			null_ncpy(unamebuf, name, (size_t) (uendp - name));
			get_hdir(unamebuf, localbuf);
			name = uendp;
		}
#endif
#ifndef MSFILESYSTEM
	} else if (name[0] == '\\' && name[1] == '~') {
		/* allow quoting of ~ (but \ is a path separator in MSDOS) */
		name += 1;
#endif /* MSFILESYSTEM */
	}
	(void) strcat(localbuf, name);

	/* Make path absolute, and prepare for processing each component
	 * of the path by placing prefix in intobuf.
	 */
#ifndef MSFILESYSTEM
	strcpy(intobuf, localbuf[0] == '/'? "/" : PWD);
#else /* MSFILESYSTEM */
	/* Convert to an absolute path, and then fudge thing so that the
	 * generic code does not have to deal with drive specifications.
	 * If the path starts with '//' it is a UNC name. Otherwise,
	 * our absolute path starts with a d: drive specification and
	 * uses forward slashes as the path separator (including one
	 * right after the drive specification).
	 */
	abspath(localbuf, intobuf);
	if (localbuf[0] == '/' && localbuf[1] == '/') {
		strcpy(localbuf, intobuf+1);
		intobuf += 1;
		intobuf[1] = '\0';
	} else {
		strcpy(localbuf, intobuf+3);		/* copy back all but d:/ */
		intobuf += 2;	/* "forget" drive spec: point to / */
		intobuf[1] = '\0';	/* truncate after d:/ */
	}
#endif /* MSFILESYSTEM */

	/* Process each path component, attempting to make the path canonical. */
	{
		char
			*fp = localbuf,	/* start of current component */
			*dp = intobuf;	/* current end of resulting path (but lazy) */
#ifdef HAS_SYMLINKS
		int	linkcnt = 0;	/* to detect symlink loops */
#endif

		while (*fp != '\0') {
			/* for each path component: */
			char	*sp = strchr(fp, '/');	/* end of current component */

			if (sp != NULL)
				*sp = '\0';
			dp += strlen(dp);	/* move to end of resulting path */
			if (*fp == '\0' || strcmp(fp, ".") == 0) {
				/* ignore this component */
			} else if (strcmp(fp, "..") == 0) {
				/* Strip one directory name from "intobuf".
				 * Assume that intobuf[0] == '/'.
				 * Symlinks are the only hard part.
				 * ??? is this correct for the Mac?  CP/M?
				 */
#ifdef HAS_SYMLINKS
				char	linkbuf[FILESIZE];
				int	linklen;
#endif

				do {} while (dp > intobuf+1 && *--dp != '/');
#ifdef HAS_SYMLINKS
				/* If we find that the path up to the .. is a symlink,
				 * and we don't appear to be in a symlink loop
				 * and we have room to handle it,
				 * we jam the symlink's target on the front of fp
				 * and try again.
				 * Note: this code will only work for UNIX-like pathnames.
				 */
				if (sp != NULL)
					*sp = '/';
				linklen = readlink(intobuf, linkbuf, sizeof(linkbuf)-1);
				if (linklen >= 0  && ++linkcnt < 100
				&& strlen(fp) + linklen + (linkbuf[0]=='/'? 0 : dp - intobuf) + 2 <= sizeof(linkbuf))
				{
					if (linklen <= 1 || linkbuf[linklen-1] != '/')
						linkbuf[linklen++] = '/';
					strcpy(&linkbuf[linklen], fp);
					strcpy(localbuf, linkbuf);
					fp = localbuf;
					if (linkbuf[0] == '/') {
						fp += 1;
						dp = &intobuf[0];
						*dp++ = '/';
					}
					*dp = '\0';
					continue;
				}
#endif
				*dp = '\0';
			} else {
				if (dp!=intobuf && dp[-1]!='/')
					*dp++ = '/';
				strcpy(dp, fp);
			}
			if (sp == NULL)
				break;

			fp = sp + 1;
		}
	}
}

#ifdef UNIX
int	CreatMode = DFLT_MODE;	/* VAR: default mode for creat'ing files */
#endif

private void 
DoWriteReg (bool app)
{
	char	fnamebuf[FILESIZE];
	Mark	*mp = CurMark();
	File	*fp;

	/* Won't get here if there isn't a Mark */
	(void) ask_file((char *)NULL, (char *)NULL, fnamebuf);

	if (!app) {
		filemunge(fnamebuf);
		chk_divergence((Buffer *)NULL, fnamebuf, "write-region");
#ifdef BACKUPFILES
		if (BkupOnWrite)
			file_backup(fnamebuf);
#endif
	}

	fp = open_file(fnamebuf, iobuff, app ? F_APPEND|F_TELLALL : F_WRITE|F_TELLALL, YES);
	putreg(fp, mp->m_line, mp->m_char, curline, curchar, YES);
	close_file(fp);
}

void 
WrtReg (void)
{
	DoWriteReg(NO);
}

void 
AppReg (void)
{
	DoWriteReg(YES);
}

bool	OkayBadChars = NO;	/* VAR: allow bad characters in filenames created by JOVE */

void 
JWriteFile (void)
{
	char
		fnamebuf[FILESIZE];

#ifdef MAC
	if (Macmode) {
		if (pfile(fnamebuf) == NULL)
			return;
	} else
#endif /* MAC */
		(void) ask_file((char *)NULL, curbuf->b_fname, fnamebuf);
	/* Don't allow bad characters when creating new files. */
	if (!OkayBadChars
	&& (curbuf->b_fname==NULL || strcmp(curbuf->b_fname, fnamebuf) != 0))
	{
#ifdef UNIX
		static const char	badchars[] = "!$^&*()~`{}\"'\\|<>? ";
#endif
#ifdef MSDOS
		static const char	badchars[] = "*|<>? ";
#endif
#ifdef WIN32
		static const char	badchars[] = "*|<>?\"";
#endif
#ifdef MAC
		static const char	badchars[] = ":";
#endif
		register char	*cp = fnamebuf;
		register char	c;

		while ((c = *cp++) != '\0') {
			if (!jisprint(c) || strchr(badchars, c)!=NULL) {
				complain("'%p': bad character in filename.", c);
				/* NOTREACHED */
			}
		}
	}

	filemunge(fnamebuf);
	chk_divergence(curbuf, fnamebuf, "write");
	curbuf->b_type = B_FILE;	/* in case it wasn't before */
	setfname(curbuf, fnamebuf);
	file_write(fnamebuf, NO);
}

void 
WtModBuf (void)
{
	if (!ModBufs(NO))
		message("[No buffers need saving]");
	else
		put_bufs(is_an_arg());
}

void 
put_bufs (bool askp)
{
	register Buffer	*oldb = curbuf,
			*b;

	for (b = world; b != NULL; b = b->b_next) {
		if (!IsModified(b) || b->b_type != B_FILE)
			continue;
		SetBuf(b);	/* Make this current Buffer */
		if (curbuf->b_fname == NULL) {
			const char	*newname;

			newname = ask(NullStr, "Buffer \"%s\" needs a file name; type Return to skip: ", b->b_name);
			if (*newname == '\0')
				continue;

			setfname(b, newname);
		}
		if (askp && !yes_or_no_p("Write %s? ", curbuf->b_fname))
			continue;

		SaveFile();
	}
	SetBuf(oldb);
}

/* Open file FNAME supplying the buffer IO routine with buffer BUF.
 * HOW is F_READ, F_WRITE or F_APPEND.  HOW can have the F_TELLALL
 * flag to request the displaying of I/O status.  Only if COMPLAINIFBAD
 * will a complain diagnostic be produced for a failed open.
 *
 * NOTE:  This opens the pr_name(fname, NO) of fname.  That is, FNAME
 *	  is usually an entire pathname, which can be slow when the
 *	  pathname is long and there are lots of symbolic links along
 *	  the way (which has become very common in my experience).  So,
 *	  this speeds up opens file names in the local directory.  It
 *	  will not speed up things like "../scm/foo.scm" simply because
 *	  by the time we get here that's already been expanded to an
 *	  absolute pathname.  But this is a start.
 */
File *
open_file (register char *fname, char *buf, register int how, bool complainifbad)
{
	register File	*fp;

	io_chars = 0;
	io_lines = 0;

	fp = f_open(pr_name(fname, NO), how, buf, LBSIZE);
	if (fp == NULL) {
		if (complainifbad) {
			message(IOerr((F_MODE(how) == F_READ) ? "open" : "create",
			    fname));
			complain((char *)NULL);
			/* NOTREACHED */
		}
	} else {
		const char	*rd_only = NullStr;

#ifndef MAC
		if (access(pr_name(fname, NO), W_OK) == -1 && errno != ENOENT) {
			rd_only = " [Read only]";
			fp->f_flags |= F_READONLY;
		}
#endif
		if (how & F_TELLALL)
			f_mess("\"%s\"%s", pr_name(fname, YES), rd_only);
	}
	return fp;
}

/* We're about to write to a file (save-file, write-region, append-region,
 * or write-file):  query user when it is an existing but different file.
 * Note: even if we are doing an append-region or write-region,
 * we assume that the current buffer's file is fair game.
 */
private void 
filemunge (char *newname)
{
	if (do_stat(newname, curbuf, DS_NONE) != curbuf && was_file) {
		rbell();
		confirm("\"%s\" already exists; overwrite it? ", newname);
		/* in case user has fiddled some more, refresh stat cache */
		(void) do_stat(newname, (Buffer *)NULL, DS_NONE);
	}
}

/* Check to see if the file has been modified since it was
 * last visited or saved.  If so, make sure they know what
 * they're doing.  Buffer "thisbuf" is tested for divergence;
 * if thisbuf is NULL, the first buffer with the file is tested.
 *
 * To avoid excessive stats, we presume that the stat cache is
 * already primed.  We refresh it if we get a confirmation because
 * it left  the user a window of opportunity for fiddling.
 */

private void 
chk_divergence (Buffer *thisbuf, const char *fname, const char *how)
{
	static const char	mesg[] = "Shall I go ahead and %s anyway? ";
	Buffer	*buf = do_stat(fname, thisbuf, DS_REUSE);

	if (buf != NULL && buf->b_diverged) {
		rbell();
		redisplay();	/* Ring that bell! */
		TOstart("Warning");
		Typeout("\"%s\" now saved on disk is not what you last", pr_name(fname, YES));
		Typeout("visited or saved.  Probably someone else is editing");
		Typeout("your file at the same time.");
		Typeout(NullStr);
		Typeout("Type \"y\" if I should %s, anyway.", how);
		f_mess(mesg, how);
		TOstop();
		confirm(mesg, how);
		/* in case user has fiddled some more, refresh stat cache */
		(void) do_stat(fname, (Buffer *)NULL, DS_NONE);
	}
}

void 
file_write (char *fname, bool app)
{
	File	*fp;

#ifdef BACKUPFILES
	if (!app && BkupOnWrite)
		file_backup(fname);
#endif

	fp = open_file(fname, iobuff, app ? F_APPEND|F_TELLALL : F_WRITE|F_TELLALL, YES);

	if (EndWNewline) {	/* Make sure file ends with a newLine */
		Bufpos	save;

		DOTsave(&save);
		ToLast();
		if (length(curline))	/* Not a blank Line */
			LineInsert(1);
		SetDot(&save);
	}
	putreg(fp, curbuf->b_first, 0, curbuf->b_last, length(curbuf->b_last), NO);
	close_file(fp);
	(void) do_stat(curbuf->b_fname, curbuf, DS_SET);
	unmodify();
}

void 
JReadFile (void)
{
	char
		fnamebuf[FILESIZE];
	bool
		reloading;
	Window
		*wp;
	long
		curlineno;

#ifdef MAC
	if (Macmode) {
		if (gfile(fnamebuf) == NULL)
			return;
	} else
#endif /* MAC */
		(void) ask_file((char *)NULL, curbuf->b_fname, fnamebuf);

	if (IsModified(curbuf)
	&& yes_or_no_p("Shall I make your changes to \"%s\" permanent? ", curbuf->b_name))
		SaveFile();

	(void) do_stat(fnamebuf, (Buffer *)NULL, DS_NONE);	/* prime stat cache */
	chk_divergence(curbuf, fnamebuf, "read");

	reloading = do_stat(fnamebuf, curbuf, DS_REUSE) == curbuf;

	/* preserve w_line in each window into curbuf */
	wp = fwind;
	do {
		if (wp->w_bufp == curbuf) {
			/* hijack w_topnum -- nobody was using it anyway */
			wp->w_topnum = reloading? LinesTo(curbuf->b_first, wp->w_line) : 0;
			wp->w_top = wp->w_line = NULL;
			wp->w_flags |= W_TOPGONE;
		}
	} while ((wp = wp->w_next) != fwind);

	curlineno = reloading? LinesTo(curbuf->b_first, curline) : 0;

	buf_clear(curbuf);
	setfname(curbuf, fnamebuf);
	read_file(fnamebuf, NO);

	/* recover dot in each window into curbuf */
	wp = fwind;
	do {
		if (wp->w_bufp == curbuf) {
			wp->w_top = curbuf->b_first;
			wp->w_line = next_line(curbuf->b_first, wp->w_topnum);
		}
	} while ((wp = wp->w_next) != fwind);

	SetLine(next_line(curbuf->b_first, curlineno));
}

void 
InsFile (void)
{
	char
		fnamebuf[FILESIZE];
#ifdef MAC
	if (Macmode) {
		if (gfile(fnamebuf) == NULL)
			return;
	} else
#endif /* MAC */
		(void) ask_file((char *)NULL, curbuf->b_fname, fnamebuf);
	read_file(fnamebuf, YES);
}

#include "temp.h"

bool	DOLsave = NO;	/* Do Lsave flag.  If lines aren't being saved
			 * when you think they should have been, this
			 * flag is probably not being set, or is being
			 * cleared before lsave() was called.
			 */

private int	nleft,	/* number of good characters left in current block */
		tmpfd = -1;
daddr	DFree = 1;	/* pointer to end of tmp file */
private char	*tfname;	/* pathname of tempfile where buffer lines go */

private void 
tmpinit (void)
{
	char	buf[FILESIZE];

	PathCat(buf, sizeof(buf), TmpDir,
#ifdef MAC
		".joveXXX"	/* must match string in mac.c:Ffilter() */
#else
		"jvXXXXXX"
#endif
		);
	tfname = copystr(buf);
	tmpfd = MakeTemp(tfname, "editing");
#ifdef RECOVER
	rectmpname(strrchr(tfname, '/') + 1);
#endif
}

/* Close tempfile before execing a child process.
 * Since we might be vforking, we must not change any variables
 * (in particular tmpfd).
 */
void 
tmpclose (void)
{
	if (tmpfd != -1)
		(void) close(tmpfd);
}

/* Close and remove tempfile before exiting. */

void 
tmpremove (void)
{
	if (tmpfd != -1) {
		tmpclose();
		(void) unlink(tfname);
	}
}

/* get a line at `tl' in the tmp file into `buf' which should be LBSIZE
 * long
 */

/* A prototyped definition is needed because daddr might be affected
 * by default argument promotions.
 */

int	Jr_Len;		/* length of Just Read Line */

void
#ifdef USE_PROTOTYPES
jgetline proto((daddr addr, register char *buf))
#else
jgetline(addr, buf)
daddr	addr;
register char	*buf;
#endif
{
	register char	*bp,
			*lp;

	lp = buf;
	bp = getblock(addr, NO);
	do {} while ((*lp++ = *bp++) != '\0');
	Jr_Len = (lp - buf) - 1;
}

/* Put `buf' and return the disk address */

daddr 
jputline (char *buf)
{
	register char	*bp,
			*lp;
	register int	nl;
	daddr	line_daddr;

	lp = buf;
	bp = getblock(DFree, YES);
	nl = nleft;
	while ((*bp = *lp++) != '\0') {
		if (*bp++ == '\n') {
			*--bp = '\0';
			break;
		}
		if (--nl == 0) {
			DFree = blk_chop(DFree) + BLK_CHNKS;
			bp = getblock(DFree, YES);
			lp = buf;	/* start over ... */
			nl = nleft;
		}
	}
	line_daddr = DFree;
	DFree += REQ_CHNKS(lp - buf);	/* (lp - buf) includes the null */
	return line_daddr;
}

/* The theory is that the critical section of code inside this procedure
 * will never cause a problem to occur.  Basically, we need to ensure
 * that two blocks are in memory at the same time, but I think that
 * this can never screw up.
 */
#define lockblock(addr)
#define unlockblock(addr)

private bool 
f_getputl (LinePtr line, register File *fp)
{
	register char	*bp;
	register ZXchar	c;
	register int
			nl,
			room = LBSIZE-1;
	char		*base;

	base = bp = getblock(DFree, YES);
	nl = nleft;
	for (;;) {
		c = f_getc(fp);
		if (c == EOF)
			break;

		/* We can't store NUL in our buffer, so ignore it.
		 * Similarly, we can only store characters less than NCHARS.
		 * Of course, with a little ingenuity we could store NUL:
		 * NUL could be represented by \n.
		 */
		if (c == '\0'
#if NCHARS != UCHAR_ROOF
		|| c >= NCHARS
#endif
		)
			continue;

		if (c == EOL) {
#ifdef USE_CRLF
			/* a CR followed by an EOL is treated as a NL.
			 * Bug: the line-buffer is effectively shortened by one character.
			 */
			if (bp != base && bp[-1] == '\r') {
				bp -= 1;
				room += 1;
			}
#endif /* USE_CRLF */
			break;
		}
		if (--room < 0)
			break;	/* no room for this character */

		if (--nl == 0) {
			char	*newbp;
			size_t	nbytes;
			daddr		old_free_block = blk_chop(DFree);

			lockblock(old_free_block);
			DFree = old_free_block + BLK_CHNKS;
			nbytes = bp - base;
			newbp = getblock(DFree, YES);
			nl = nleft;
			byte_copy(base, newbp, nbytes);
			bp = newbp + nbytes;
			base = newbp;
			unlockblock(old_free_block);
		}
		*bp++ = c;
	}
	*bp++ = '\0';
	line->l_dline = DFree;
	DFree += REQ_CHNKS(bp - base);
	if (room < 0) {
		add_mess(" [Line too long]");
		rbell();
		return YES;
	}
	if (c == EOF) {
		if (--bp != base)
			add_mess(" [Incomplete last line]");
		return YES;
	}
	io_lines += 1;
	return NO;
}

typedef struct block {
	char	b_dirty;	/* (bool) */
	daddr	b_bno;
	char	b_buf[JBUFSIZ];
	struct block
		*b_LRUnext,
		*b_LRUprev,
		*b_HASHnext;
} Block;

#define HASHSIZE	7	/* Primes work best (so I'm told) */
#define B_HASH(bno)	((bno) % HASHSIZE)

#ifdef MALLOC_CACHE
private Block	*b_cache = NULL;
#else
private Block	b_cache[NBUF];
#endif

private Block
	*bht[HASHSIZE],		/* Block hash table. Must be zero initially */
	*f_block = NULL,
	*l_block = NULL;

private daddr	next_bno = 0;

/* Needed to comfort MS Visual C */
private void blkio proto((Block *, JSSIZE_T (*) ptrproto((int, UnivPtr, size_t))));

private void
blkio(b, iofcn)
register Block	*b;
register JSSIZE_T	(*iofcn) ptrproto((int, UnivPtr, size_t));
{
	off_t boff = bno_to_seek_off(b->b_bno);
	JSSIZE_T nb;
	static bool first_time = YES;

	if (first_time) {
		tmpinit();
		first_time = NO;
	}
	if (lseek(tmpfd, boff, 0) < 0) {
		error("[Tmp file seek error to %D: %d %s; to continue editing would be dangerous]",
		      (long)boff, errno, strerror(errno));
		/* NOTREACHED */
	}
	else if ((nb = (*iofcn)(tmpfd, (UnivPtr) b->b_buf, (size_t)JBUFSIZ)) != JBUFSIZ) {
		error("[Tmp file %s error got %D: %d %s: to continue editing would be dangerous]",
			(iofcn == read) ? "READ" : "WRITE", (long)nb,
			nb < 0 ? errno : 0, nb < 0 ? strerror(errno): "");
		/* NOTREACHED */
	}
}

void 
d_cache_init (void)
{
	register Block	*bp,	/* Block pointer */
			**hp;	/* Hash pointer */
	register daddr	bno;

	jdbg("MAX_BLOCKS=%D\n", (long)MAX_BLOCKS);
	jdbg("CHNK_CHARS=%D\n", (long)CHNK_CHARS);
	jdbg("BLK_CHNKS=%D\n", (long)BLK_CHNKS);
#ifdef MALLOC_CACHE
	if (b_cache == NULL) {
		b_cache = (Block *) calloc((size_t)NBUF,sizeof(Block));
		if (b_cache == NULL) {
			error("cannot allocate buffer cache");
			/* NOTREACHED */
		}
	}
#endif /* MALLOC_CACHE */

	for (bp = b_cache, bno = NBUF; bno-- > 0; bp++) {
		bp->b_dirty = NO;
		bp->b_bno = bno;
		if (l_block == NULL)
			l_block = bp;
		bp->b_LRUprev = NULL;
		bp->b_LRUnext = f_block;
		if (f_block != NULL)
			f_block->b_LRUprev = bp;
		f_block = bp;

		bp->b_HASHnext = *(hp = &bht[B_HASH(bno)]);
		*hp = bp;
	}
}

void 
SyncTmp (void)
{
	register Block	*b;
#ifdef MSDOS
	register daddr	bno = 0;

	/* sync the blocks in order, for file systems that don't allow
	 * holes (MSDOS).  Perhaps this benefits floppy-based file systems.
	 */
	for (bno = 0; bno < next_bno; bno++) {
		if ((b = lookup_block(bno)) != NULL && b->b_dirty) {
			blkio(b, (JSSIZE_T (*) ptrproto((int, UnivPtr, size_t)))write);
			b->b_dirty = NO;
		}
	}
#else /* !MSDOS */
	for (b = f_block; b != NULL; b = b->b_LRUnext)
		if (b->b_dirty) {
			blkio(b, (JSSIZE_T (*) ptrproto((int, UnivPtr, size_t)))write);
			b->b_dirty = NO;
		}
#endif /* !MSDOS */
}

/* A prototyped definition is needed because daddr might be affected
 * by default argument promotions.
 */

private Block *
#ifdef USE_PROTOTYPES
lookup_block proto((register daddr bno))
#else
lookup_block(bno)
register daddr	bno;
#endif
{
	register Block	*bp;

	for (bp = bht[B_HASH(bno)]; bp != NULL; bp = bp->b_HASHnext)
		if (bp->b_bno == bno)
			break;
	return bp;
}

private void 
LRUunlink (register Block *b)
{
	if (b->b_LRUprev == NULL)
		f_block = b->b_LRUnext;
	else
		b->b_LRUprev->b_LRUnext = b->b_LRUnext;
	if (b->b_LRUnext == NULL)
		l_block = b->b_LRUprev;
	else
		b->b_LRUnext->b_LRUprev = b->b_LRUprev;
}

private Block *
b_unlink (register Block *bp)
{
	register Block	*hp,
			*prev = NULL;

	LRUunlink(bp);
	/* Now that we have the block, we remove it from its position
	 * in the hash table, so we can THEN put it somewhere else with
	 * it's new block assignment.
	 */
	for (hp = bht[B_HASH(bp->b_bno)]; hp != NULL; prev = hp, hp = hp->b_HASHnext)
		if (hp == bp)
			break;
	if (hp == NULL) {
		writef("\rBlock %ld missing!", (long)bp->b_bno);
		finish(0);
	}
	if (prev)
		prev->b_HASHnext = hp->b_HASHnext;
	else
		bht[B_HASH(bp->b_bno)] = hp->b_HASHnext;

	if (bp->b_dirty) {	/* do, now, the delayed write */
		blkio(bp, (JSSIZE_T (*) ptrproto((int, UnivPtr, size_t)))write);
		bp->b_dirty = NO;
	}

	return bp;
}

/* Get a block which contains at least part of the line with the address
 * atl.  Returns a pointer to the block and sets the global variable
 * nleft (number of good characters left in the buffer).
 */

/* A prototyped definition is needed because daddr might be affected
 * by default argument promotions.
 */

private char *
#ifdef USE_PROTOTYPES
getblock proto((daddr atl, bool IsWrite))
#else
getblock(atl, IsWrite)
daddr	atl;
bool	IsWrite;
#endif
{
	register daddr	bno,
			off;
	register Block	*bp;
	static Block	*lastb = NULL;

	bno = da_to_bno(atl);
	off = da_to_off(atl);
	/* We don't allow block number MAX_BLOCKS-1 to be used because
	 * NOWHERE_DADDR and NOTYET_DADDR must not be valid disk references,
	 * and we want to prevent space overflow from being undetected
	 * through arithmetic overflow.
	 */
	if (bno >=  MAX_BLOCKS-1) {
		error("Tmp file too large for line %D bno %D, max is %D.  Get help!", (long)atl, (long)bno, (long)(MAX_BLOCKS-1));
		/* NOTREACHED */
	}
	nleft = JBUFSIZ - off;
	if (lastb != NULL && lastb->b_bno == bno) {
		bp = lastb;	/* same as last time */
	} else if ((bp = lookup_block(bno)) != NULL) {
		/* The requested block already lives in memory, so we move
		 * it to the end of the LRU list (making it Most Recently Used)
		 * and then return a pointer to it.
		 */
		if (bp != l_block) {
			LRUunlink(bp);
			if (l_block == NULL)
				f_block = bp;
			else
				l_block->b_LRUnext = bp;
			bp->b_LRUprev = l_block;
			l_block = bp;
			bp->b_LRUnext = NULL;
		}
		if (bno >= next_bno)
			next_bno = bno + 1;
	} else {
		/* The block we want doesn't reside in memory so we take the
		 * least recently used clean block (if there is one) and use
		 * it.
		 */
		bp = f_block;
		if (bp->b_dirty)	/* The best block is dirty ... */
			SyncTmp();

		bp = b_unlink(bp);
		if (l_block == NULL)
			f_block = bp;
		else
			l_block->b_LRUnext = bp;	/* Place it at the end ... */
		bp->b_LRUprev = l_block;
		l_block = bp;
		bp->b_LRUnext = NULL;		/* so it's Most Recently Used */

		bp->b_dirty = NO;
		bp->b_bno = bno;
		bp->b_HASHnext = bht[B_HASH(bno)];
		bht[B_HASH(bno)] = bp;

		/* Get the current contents of the block UNLESS this is a new
		 * block that's never been looked at before, i.e., it's past
		 * the end of the tmp file.
		 */

		if (bno < next_bno)
			blkio(bp, read);
		else
			next_bno = bno + 1;
	}
	lastb = bp;
	bp->b_dirty |= IsWrite;
	return bp->b_buf + off;
}

char *
lbptr (LinePtr line)
{
	return getblock(line->l_dline, NO);
}

/* save the current contents of linebuf, if it has changed */

void 
lsave (void)
{
	if (curbuf == NULL || !DOLsave)	/* Nothing modified recently */
		return;

	if (strcmp(lbptr(curline), linebuf) != 0)
		SavLine(curline, linebuf);	/* Put linebuf on the disk. */
	DOLsave = NO;
}

/* build backup file name, also used by SetBuf */
void
backup_name(fname, btype, bfname, bfnamesize)
const char	*fname,
		*btype;
char		*bfname;
size_t		bfnamesize;
{
	char	*s = strrchr(fname, '/');
	size_t	dirlen = (s == NULL)? 0 : s + 1 - fname;

	jamstrsub(bfname, fname, bfnamesize);
	swritef(bfname+dirlen, (size_t) (bfnamesize - dirlen), "#%s%s~",
		fname+dirlen, btype);
}

#ifdef BACKUPFILES
private void 
file_backup (char *fname)
{
# ifndef MSFILESYSTEM
	JSSIZE_T	rr;
	int
		ffd,
		bffd = 0;	/* avoid uninitialized complaint from gcc -W */
	char
		buf[JBUFSIZ],
		bfname[FILESIZE];

	backup_name(fname, "", bfname, sizeof(bfname));

	if ((ffd = open(fname, O_RDONLY | O_BINARY | O_CLOEXEC)) < 0)
		return;	/* cannot open original file: nothing to backup, we assume */

	/* create backup file with same mode as input file */
	{
#  ifdef MAC
		jmode_t	mode = CreatMode;	/* dummy */
#  else
		struct stat statbuf;
		jmode_t	mode = fstat(ffd, &statbuf) != 0?
			(jmode_t)CreatMode : statbuf.st_mode;
#  endif

		/* Unlink the pathname before creating.  It may have been
		 * created by someone else, or worse it may be linked or
		 * symlinked somewhere else, none of which we want to
		 * overwrite.  There is still a possible race-condition,
		 * so we need to use O_EXCL if we can.
		 */
		if ((unlink(bfname) < 0 && errno != ENOENT)
#  ifdef O_EXCL
		|| (bffd = open(bfname, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, mode)) < 0
#  else
		|| (bffd = creat(bfname, mode)) < 0
#  endif
		) {
			int	e = errno;

			(void) close(ffd);
			complain("[cannot create backup \"%s\": %d %s]",
				bfname, e, strerror(e));
			/* NOTREACHED */
		}
	}

	/* copy the contents */
	while ((rr = read(ffd, (UnivPtr) buf, sizeof(buf))) > 0) {
		char	*p = buf;

		while (rr > 0) {
			JSSIZE_T	wr = write(bffd, (UnivPtr) p, (size_t) rr);

			if (wr < 0) {
				int e = errno;

				close(bffd);
				close(ffd);
				complain("[error writing backup: %d %s]", e, strerror(e));
				/* NOTREACHED */
			}
			p += wr;
			rr -= wr;
		}
	}

	if (rr < 0 || close(ffd) != 0) {
		complain("[error reading \"%s\": %d %s]", fname, errno, strerror(errno));
		/* NOTREACHED */
	}
#  ifdef USE_FSYNC
	if (fsync(bffd) != 0) {
		int	e = errno;

		(void) close(bffd);
		complain("[error fsyncing backup: %d %s]", e, strerror(e));
		/* NOTREACHED */
	}
#  endif /* USE_FSYNC */
	if (close(bffd) != 0) {
		complain("[error closing backup: %d %s]", errno, strerror(errno));
		/* NOTREACHED */
	}
# else /* MSFILESYSTEM */
	/* This code is designed to fit withing the 8.3 limitation of
	 * MSDOS ("FAT" -- huh!) file systems.  Even though newer versions
	 * of these APIs (Win32) may support longer file names, we may still
	 * be dealing with a FAT file system.
	 */
	char	*dot,
			*slash,
			tmp[FILESIZE];

	if (access(fname, 0) < 0)
		return;	/* cannot open original file: nothing to backup, we assume */

	strcpy(tmp, fname);
	slash = (char *)jbasename(tmp);
	if ((dot = strrchr(slash, '.')) != NULL) {
		if (stricmp(dot,".bak") == 0)
			return;	/* don't rename .bak to .bak */
		*dot = '\0';
	}
	strcat(tmp, ".bak");
	unlink(tmp);
	if (rename(fname, tmp) != 0) {
		complain("[cannot rename to \"%s\": %s]", tmp, strerror(errno));
		/* NOTREACHED */
	}
# endif /* MSFILESYSTEM */
}
#endif /* BACKUPFILES */
