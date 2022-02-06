/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

/* macros for getting at and setting the current argument count */

#define arg_value()		arg_count
#define arg_value_as_unsigned()	arg_count_as_unsigned(__FILE__, __LINE__)
#define arg_value_as_int()	arg_count_as_int(__FILE__, __LINE__)
#define arg_value_as_intptr_t()	arg_count_as_intptr_t(__FILE__, __LINE__)

#define arg_or_default(x)	(is_an_arg()? arg_count : (x))
#define arg_or_default_as_unsigned(x)	(is_an_arg()? arg_value_as_unsigned() : (x))
#define arg_or_default_as_int(x)	(is_an_arg()? arg_value_as_int() : (x))
#define arg_or_default_as_intptr_t(x)	(is_an_arg()? arg_value_as_intptr_t() : (x))

#define set_arg_value(n)	{ arg_state = AS_NUMERIC; arg_count = (n); }
#define clr_arg_value()		{ arg_state = AS_NONE; arg_count = 1; }
#define is_an_arg()		(arg_state != AS_NONE)
#define is_non_minus_arg()		(arg_state != AS_NONE && arg_state != AS_NEGSIGN)

#define	save_arg(as,ac)	{ (ac) = arg_count; (as) = arg_state; }
#define	restore_arg(as,ac)	{ arg_count = (ac); arg_state = (as); }

extern void	negate_arg proto((void));

/* Commands: */

extern void
	Digit proto((void)),
	DigitMinus proto((void)),
	Digit0 proto((void)),
	Digit1 proto((void)),
	Digit2 proto((void)),
	Digit3 proto((void)),
	Digit4 proto((void)),
	Digit5 proto((void)),
	Digit6 proto((void)),
	Digit7 proto((void)),
	Digit8 proto((void)),
	Digit9 proto((void)),
	TimesFour proto((void));

extern unsigned
	arg_count_as_unsigned proto((char *file, int line));

extern int
	arg_count_as_int proto((char *file, int line));

extern INTPTR_T
	arg_count_as_intptr_t proto((char *file, int line));

/* private to macros */

extern int arg_state;	/* NO, YES, or YES_NODIGIT */
extern long arg_count;

#define	AS_NONE	0	/* no arg */
#define	AS_NUMERIC	1	/* numeric arg supplied */
#define	AS_NEGSIGN	2	/* only minus sign supplied */
#define	AS_TIMES	3	/* multiplicative request */
