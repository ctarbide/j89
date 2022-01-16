/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"

#ifdef RECOVER	/* the body is the rest of this file */

#include "fp.h"
#include "sysprocs.h"
#include "rec.h"
#include "fmt.h"
#include "recover.h"

private int	rec_fd = -1;
private char	*recfname;
private File	*rec_out;

#define dmpobj(obj) fputnchar((char *) &obj, (int) sizeof(obj), rec_out)

#ifndef L_SET
# define L_SET 0
#endif

private struct rec_head	Header;

void 
rectmpname (char *tfname)
{
	if (strlen(tfname) >= sizeof(Header.TmpFileName)) {
		complain("temporary filename too long; recovery disabled.");
		/* NOTREACHED */
	}
	strcpy(Header.TmpFileName, tfname);
}

private void 
recinit (void)
{
	char	buf[FILESIZE];

	PathCat(buf, sizeof(buf), TmpDir,
#ifdef MAC
		".jrecXXX"	/* must match string in mac.c:Ffilter() */
#else
		"jrXXXXXX"
#endif
		);
	recfname = copystr(buf);
	rec_fd = MakeTemp(recfname, "recovery (disabling it)");
	/* initialize the recovery file */
	rec_out = fd_open(recfname, F_WRITE|F_LOCKED, rec_fd, iobuff, LBSIZE);

	/* Initialize the record header (TmpFileName initialized by rectmpname). */
	Header.RecMagic = RECMAGIC;
	Header.Uid = getuid();
	Header.Pid = getpid();
}

/* Close recfile before execing a child process.
 * Since we might be vforking, we must not change any variables
 * (in particular rec_fd).
 */
void 
recclose (void)
{
	if (rec_fd != -1)
		(void) close(rec_fd);
}

/* Close and remove recfile before exiting. */


void 
recremove (void)
{
	if (rec_fd != -1) {
		recclose();
		(void) unlink(recfname);
	}
}

/* Write out the line pointers for buffer B. */

private void 
dmppntrs (register Buffer *b)
{
	register LinePtr	lp;

	for (lp = b->b_first; lp != NULL; lp = lp->l_next)
		dmpobj(lp->l_dline);
}

/* dump the buffer info and then the actual line pointers. */

private void 
dmp_buf_header (register Buffer *b)
{
	struct rec_entry	record;

	byte_zero(&record, sizeof(struct rec_entry));	/* clean out holes for purify */
	record.r_dotline = LinesTo(b->b_first, b->b_dot);
	record.r_dotchar = b->b_char;
	record.r_nlines = record.r_dotline + LinesTo(b->b_dot, (LinePtr)NULL);
	strcpy(record.r_fname, b->b_fname ? b->b_fname : NullStr);
	null_ncpy(record.r_bname, b->b_name, sizeof(record.r_bname) - 1);
	dmpobj(record);
}

/* Goes through all the buffers and syncs them to the disk. */

int	ModCount = 0;	/* number of buffer mods since last sync */

int	SyncFreq = 50;	/* VAR: how often to sync the file pointers */

void 
SyncRec (void)
{
	register Buffer	*b;
	static bool	beenhere = NO;
	time_t		tupd;

	/* Count number of interesting buffers.  If none, don't bother syncing. */
	Header.Nbuffers = 0;
	for (b = world; b != NULL; b = b->b_next)
		if (b->b_type != B_SCRATCH && IsModified(b))
			Header.Nbuffers += 1;
	if (Header.Nbuffers == 0)
		return;

	lsave();	/* this makes things really right */
	SyncTmp();	/* note: this will force rectmpname() */

	if (!beenhere) {
		beenhere = YES;
		recinit();	/* Init recover file. */
	}
	/* Note: once writing to the recover file fails, we permanently
	 * stop trying.  This is to avoid useless thrashing.  Perhaps
	 * there should be a way to turn this back on.
	 */
	if (rec_fd == -1 || (rec_out->f_flags & F_ERR))
		return;

	f_seek(rec_out, (off_t)0);
	(void) time(&tupd);
	Header.UpdTime = tupd;
	Header.FreePtr = DFree;
	dmpobj(Header);
	for (b = world; b != NULL; b = b->b_next)
		if (b->b_type != B_SCRATCH && IsModified(b))
			dmp_buf_header(b);
	for (b = world; b != NULL; b = b->b_next)
		if (b->b_type != B_SCRATCH && IsModified(b))
			dmppntrs(b);
	flushout(rec_out);
}

/* To be implemented:
 * Full Recover.  What we have to do is go find the name of the tmp
 * file data/rec pair and use those instead of the ones we would have
 * created eventually.  The rec file has a list of buffers, and then
 * the actual pointers.  Stored for each buffer is the buffer name,
 * the file name, the number of lines, the current line, the current
 * character.  The current modes do not need saving as they will be
 * saved when the file name is set.  If a process was running in a
 * buffer, it will be lost.
 */

#endif /* RECOVER */
