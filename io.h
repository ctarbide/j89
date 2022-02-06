/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

extern const char	*HomeDir;

extern size_t	HomeLen;

extern bool	DOLsave;	/* Do Lsave flag.  If lines aren't being saved
				   when you think they should have been, this
				   flag is probably not being set, or is being
				   cleared before lsave() was called. */

extern daddr	DFree;	/* pointer to end of tmp file */

extern int	Jr_Len;		/* length of Just Read Line */

extern long
io_chars,
io_lines;

extern int	MakeTemp proto((char *, const char *));

extern char
	*lbptr proto((LinePtr line)),
	*pr_name proto((const char *fname, bool okay_home)),
	*pwd proto((void));

extern File
	*open_file proto((char *fname, char *buf, int how, bool complainifbad));

extern void
	setCWD proto((char *d)),
	getCWD proto((void)),
	PathCat proto((char *buf, size_t, const char *pre, const char *post)),
	PathParse proto((const char *name, char *intobuf)),
	SyncTmp proto((void)),
	close_file proto((File *fp)),
	d_cache_init proto((void)),
	file_write proto((char *fname, bool app)),
	jgetline proto((daddr addr, char *buf)),
	lsave proto((void)),
	putreg proto((File *fp, LinePtr line1, INTPTR_T char1, LinePtr line2, INTPTR_T char2, bool makesure)),
	read_file proto((char *file, bool is_insert)),
	put_bufs proto((bool askp)),
	tmpclose proto((void)),
	tmpremove proto((void));

extern bool
	chkCWD proto((char *dn));

extern daddr
	jputline proto((char *buf));

/* Commands: */

extern void
	AppReg proto((void)),
	Chdir proto((void)),
	InsFile proto((void)),
	Popd proto((void)),
	Pushd proto((void)),
	Pushlibd proto((void)),
	JReadFile proto((void)), /* ReadFile conflicts with Win32 library */
	SaveFile proto((void)),
	JWriteFile proto((void)), /* WriteFile conflicts with Win32 library */
	WtModBuf proto((void)),
	WrtReg proto((void)),
	prCWD proto((void)),
	prDIRS proto((void)),
	backup_name proto((const char *fname, const char *btype, char *bfname, size_t bfnamesize));

/* Variables: */

#ifdef BACKUPFILES
extern bool	BkupOnWrite;		/* VAR: make backup files when writing */
#endif
#ifdef UNIX
extern int	CreatMode;		/* VAR: default mode for creat'ing files */
#endif
#ifdef MAC
# define CreatMode	0	/* dummy */
#endif
extern bool	EndWNewline;		/* VAR: end files with a blank line */
extern bool	OkayBadChars;		/* VAR: allow bad characters in files created by JOVE */
