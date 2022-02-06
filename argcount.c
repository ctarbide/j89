/**************************************************************************
 * This program is Copyright (C) 1986-2002 by Jonathan Payne.  JOVE is    *
 * provided by Jonathan and Jovehacks without charge and without          *
 * warranty.  You may copy, modify, and/or distribute JOVE, provided that *
 * this notice is included in all the source files and documentation.     *
 **************************************************************************/

#include "jove.h"
#include "jctype.h"

int	arg_state = AS_NONE;
long	arg_count;

unsigned
arg_count_as_unsigned(char *file, int line)
{
	if ((unsigned long)arg_count > (unsigned long)UINT_MAX) {
		fprintf(stderr, "fatal: %s:%d: arg_count cannot be cast to unsigned\n", file, line);
		exit(1);
	}
	return (unsigned)arg_count;
}

int
arg_count_as_int(char *file, int line)
{
	if (arg_count < INT_MIN || arg_count > INT_MAX) {
		fprintf(stderr, "fatal: %s:%d: arg_count cannot be cast to int\n", file, line);
		exit(1);
	}
	return (int)arg_count;
}

INTPTR_T
arg_count_as_intptr_t(char *file, int line)
{
	if (arg_count < INTPTR_MIN || arg_count > INTPTR_MAX) {
		fprintf(stderr, "fatal: %s:%d: arg_count cannot be cast to intptr_t\n", file, line);
		exit(1);
	}
	return (INTPTR_T)arg_count;
}

void
negate_arg(void)
{
	if (arg_count < 0) {
		arg_count = -arg_count;

		if (arg_count < 0) {
			complain("arg count overflow");
			/* NOTREACHED */
		}
	} else {
		arg_count = -arg_count;
	}
}

private void
gather_argument(
	int ns,		/* new state */
	int nc		/* new count */
)
{
	for (;;) {
		ZXchar	c;
		bool	neg = NO;

		if (arg_count < 0) {
			neg = YES;
			negate_arg();
		}

		if (ns != arg_state) {
			/* First time in this state */
			arg_state = ns;
			arg_count = nc;	/* ignore previous value (but remember sign) */
		} else {
			/* Continuing in this state. */
			long	t = arg_count;

			switch (ns) {
			case AS_NUMERIC:
				t = t * 10 + nc;	/* add a digit to previous value */
				break;

			case AS_NEGSIGN:
				neg = !neg;	/* change previous sign */
				break;

			case AS_TIMES:
				t *= nc;	/* multiply by factor */
				break;
			}

			if (t < arg_count) {
				complain("arg count overflow");
				/* NOTREACHED */
			}

			arg_count = t;
		}

		if (neg) {
			negate_arg();
		}

		/* Treat a following digit as AS_NUMERIC.
		 * If in AS_TIMES, accept a '-'.
		 */
		c = waitchar();

		if (jisdigit(c)) {
			ns = AS_NUMERIC;
			nc = c - '0';
		} else if (arg_state == AS_TIMES && c == '-') {
			ns = AS_NEGSIGN;	/* forget multiplication */
			nc = -1;
		} else {
			Ungetc(c);
			break;
		}
	}

	this_cmd = ARG_CMD;
}

void
TimesFour(void)
{
	gather_argument(AS_TIMES, 4);
}

void
Digit(void)
{
	if (LastKeyStruck == '-') {
		gather_argument(AS_NEGSIGN, -1);
	} else if (jisdigit(LastKeyStruck)) {
		gather_argument(AS_NUMERIC, LastKeyStruck - '0');
	} else {
		complain((char *)NULL);
		/* NOTREACHED */
	}
}

void
Digit0(void)
{
	gather_argument(AS_NUMERIC, 0);
}

void
Digit1(void)
{
	gather_argument(AS_NUMERIC, 1);
}

void
Digit2(void)
{
	gather_argument(AS_NUMERIC, 2);
}

void
Digit3(void)
{
	gather_argument(AS_NUMERIC, 3);
}

void
Digit4(void)
{
	gather_argument(AS_NUMERIC, 4);
}

void
Digit5(void)
{
	gather_argument(AS_NUMERIC, 5);
}

void
Digit6(void)
{
	gather_argument(AS_NUMERIC, 6);
}

void
Digit7(void)
{
	gather_argument(AS_NUMERIC, 7);
}

void
Digit8(void)
{
	gather_argument(AS_NUMERIC, 8);
}

void
Digit9(void)
{
	gather_argument(AS_NUMERIC, 9);
}

void
DigitMinus(void)
{
	gather_argument(AS_NEGSIGN, -1);
}
