/* Scheme In One Defun, but in C this time.

 *                      COPYRIGHT (c) 1988-1994 BY                          *
 *        PARADIGM ASSOCIATES INCORPORATED, CAMBRIDGE, MASSACHUSETTS.       *
 *                         ALL RIGHTS RESERVED                              *

 Permission to use, copy, modify, distribute and sell this software
 and its documentation for any purpose and without fee is hereby
 granted, provided that the above copyright notice appear in all copies
 and that both that copyright notice and this permission notice appear
 in supporting documentation, and that the name of Paradigm Associates
 Inc not be used in advertising or publicity pertaining to distribution
 of the software without specific, written prior permission.

 PARADIGM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 PARADIGM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 SOFTWARE.

 */

/*

   gjc@world.std.com

   Paradigm Associates Inc          Phone: 617-492-6079
   29 Putnam Ave, Suite 6
   Cambridge, MA 02138


   Release 1.0: 24-APR-88
   Release 1.1: 25-APR-88, added: macros, predicates, load. With additions by
   Barak.Pearlmutter@DOGHEN.BOLTZ.CS.CMU.EDU: Full flonum recognizer,
   cleaned up uses of NULL/0. Now distributed with siod.scm.
   Release 1.2: 28-APR-88, name changes as requested by JAR@AI.AI.MIT.EDU,
   plus some bug fixes.
   Release 1.3: 1-MAY-88, changed env to use frames instead of alist.
   define now works properly. vms specific function edit.
   Release 1.4 20-NOV-89. Minor Cleanup and remodularization.
   Now in 3 files, siod.h, slib.c, siod.c. Makes it easier to write your
   own main loops. Some short-int changes for lightspeed C included.
   Release 1.5 29-NOV-89. Added startup flag -g, select stop and copy
   or mark-and-sweep garbage collection, which assumes that the stack/register
   marking code is correct for your architecture.
   Release 2.0 1-DEC-89. Added repl_hooks, Catch, Throw. This is significantly
   different enough (from 1.3) now that I'm calling it a major release.
   Release 2.1 4-DEC-89. Small reader features, dot, backquote, comma.
   Release 2.2 5-DEC-89. gc,read,print,eval, hooks for user defined datatypes.
   Release 2.3 6-DEC-89. save_forms, obarray intern mechanism. comment char.
   Release 2.3a......... minor speed-ups. i/o interrupt considerations.
   Release 2.4 27-APR-90 gen_readr, for read-from-string.
   Release 2.5 18-SEP-90 arrays added to SIOD.C by popular demand. inums.
   Release 2.6 11-MAR-92 function prototypes, some remodularization.
   Release 2.7 20-MAR-92 hash tables, fasload. Stack check.
   Release 2.8  3-APR-92 Bug fixes, \n syntax in string reading.
   Release 2.9 28-AUG-92 gc sweep bug fix. fseek, ftell, etc. Change to
   envlookup to allow (a . rest) suggested by bowles@is.s.u-tokyo.ac.jp.
   Release 2.9a 10-AUG-93. Minor changes for Windows NT.
   Release 3.0  1-MAY-94. Release it, include changes/cleanup recommended by
   andreasg@nynexst.com for the OS2 C++ compiler. Compilation and running
   tested using DEC C, VAX C. WINDOWS NT. GNU C on SPARC. Storage
   management improvements, more string functions. SQL support.
   Release 3.1? -JUN-95 verbose flag, other integration improvements for htqs.c
   hpux by denson@sdd.hp.com, solaris by pgw9@columbia.edu.
   Release 3.2X MAR-96. dynamic linking, subr closures, other improvements.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/times.h>

#include "siod.h"
#include "siodp.h"

#define MAX_ERROR 1024
char siod_err_msg[MAX_ERROR];

static void
init_slib_version (void)
{
  setvar (cintern ("*slib-version*"),
	  cintern ("$Id$"),
	  NIL);
}

char *
siod_version (void)
{
  return ("3.2x 12-MAR-96");
}

long nheaps = 2;
LISP *heaps;
LISP heap, heap_end, heap_org;
long heap_size = 5000;
long old_heap_used;
long gc_status_flag = 1;
char *init_file = (char *) NULL;
char *tkbuffer = NULL;
long gc_kind_copying = 0;
long gc_cells_allocated = 0;
double gc_time_taken;
LISP *stack_start_ptr = NULL;
LISP freelist;
jmp_buf errjmp;
long errjmp_ok = 0;
long nointerrupt = 1;
long interrupt_differed = 0;
LISP oblistvar = NIL;
LISP sym_t = NIL;
LISP eof_val = NIL;
LISP sym_errobj = NIL;
LISP sym_catchall = NIL;
LISP sym_progn = NIL;
LISP sym_lambda = NIL;
LISP sym_quote = NIL;
LISP sym_dot = NIL;
LISP sym_after_gc = NIL;
LISP sym_eval_history_ptr = NIL;
LISP unbound_marker = NIL;
LISP *obarray;
LISP repl_return_val = NIL;
long obarray_dim = 100;
struct catch_frame *catch_framep = (struct catch_frame *) NULL;
void (*repl_puts) (char *) = NULL;
LISP (*repl_read) (void) = NULL;
LISP (*repl_eval) (LISP) = NULL;
void (*repl_print) (LISP) = NULL;
LISP *inums;
long inums_dim = 256;
struct user_type_hooks *user_types = NULL;
long user_tc_next = tc_user_min;
struct gc_protected *protected_registers = NULL;
jmp_buf save_regs_gc_mark;
double gc_rt;
long gc_cells_collected;
char *user_ch_readm = "";
char *user_te_readm = "";
LISP (*user_readm) (int, struct gen_readio *) = NULL;
LISP (*user_readt) (char *, long, int *) = NULL;
void (*fatal_exit_hook) (void) = NULL;
#ifdef THINK_C
int ipoll_counter = 0;
#endif

char *stack_limit_ptr = NULL;
long stack_size =
#ifdef THINK_C
10000;
#else
50000;
#endif

long siod_verbose_level = 4;

#ifndef SIOD_LIB_DEFAULT
#define SIOD_LIB_DEFAULT "/usr/local/lib/siod"
#endif

/*  Added by Spencer Kimball for script-fu shit 6/3/97 */
FILE *siod_output = stdout;

char *siod_lib = SIOD_LIB_DEFAULT;

void
process_cla (int argc, char **argv, int warnflag)
{
  int k;
  char *ptr;
  static siod_lib_set = 0;
#if !defined(vms)
  if (!siod_lib_set)
    {
      if (getenv ("SIOD_LIB"))
	{
	  siod_lib = getenv ("SIOD_LIB");
	  siod_lib_set = 1;
	}
    }
#endif
  for (k = 1; k < argc; ++k)
    {
      if (strlen (argv[k]) < 2)
	continue;
      if (argv[k][0] != '-')
	{
	  if (warnflag)
	    fprintf (stderr, "bad arg: %s\n", argv[k]);
	  continue;
	}
      switch (argv[k][1])
	{
	case 'l':
	  siod_lib = &argv[k][2];
	  break;
	case 'h':
	  heap_size = atol (&(argv[k][2]));
	  if ((ptr = strchr (&(argv[k][2]), ':')))
	    nheaps = atol (&ptr[1]);
	  break;
	case 'o':
	  obarray_dim = atol (&(argv[k][2]));
	  break;
	case 'i':
	  init_file = &(argv[k][2]);
	  break;
	case 'n':
	  inums_dim = atol (&(argv[k][2]));
	  break;
	case 'g':
	  gc_kind_copying = atol (&(argv[k][2]));
	  break;
	case 's':
	  stack_size = atol (&(argv[k][2]));
	  break;
	case 'v':
	  siod_verbose_level = atol (&(argv[k][2]));
	  break;
	default:
	  if (warnflag)
	    fprintf (stderr, "bad arg: %s\n", argv[k]);
	}
    }
}

void
print_welcome (void)
{
  if (siod_verbose_level >= 2)
    {
      fprintf (siod_output, "Welcome to SIOD, Scheme In One Defun, Version %s\n",
	       siod_version ());
      fprintf (siod_output, "(C) Copyright 1988-1994 Paradigm Associates Inc. Help: (help)\n\n");
      fflush (siod_output);
    }
}

void
print_hs_1 (void)
{
  if (siod_verbose_level >= 2)
    {
      fprintf (siod_output, "%ld heaps. size = %ld cells, %ld bytes. %ld inums. GC is %s\n",
	       nheaps,
	       heap_size, heap_size * sizeof (struct obj),
	       inums_dim,
	       (gc_kind_copying == 1) ? "stop and copy" : "mark and sweep");
      fflush (siod_output);
    }
}

void
print_hs_2 (void)
{
  if (siod_verbose_level >= 2)
    {
      if (gc_kind_copying == 1)
	fprintf (siod_output, "heaps[0] at %p, heaps[1] at %p\n", heaps[0], heaps[1]);
      else
	fprintf (siod_output, "heaps[0] at %p\n", heaps[0]);
      fflush (siod_output);
    }
}

long
no_interrupt (long n)
{
  long x;
  x = nointerrupt;
  nointerrupt = n;
  if ((nointerrupt == 0) && (interrupt_differed == 1))
    {
      interrupt_differed = 0;
      err_ctrl_c ();
    }
  return (x);
}

void
handle_sigfpe (int sig SIG_restargs)
{
  signal (SIGFPE, handle_sigfpe);
  err ("floating point exception", NIL);
}

void
handle_sigint (int sig SIG_restargs)
{
  signal (SIGINT, handle_sigint);
  if (nointerrupt == 1)
    interrupt_differed = 1;
  else
    err_ctrl_c ();
}

void
err_ctrl_c (void)
{
  err ("control-c interrupt", NIL);
}

LISP
get_eof_val (void)
{
  return (eof_val);
}

long
repl_driver (long want_sigint, long want_init, struct repl_hooks *h)
{
  int k;
  struct repl_hooks hd;
  LISP stack_start;
  stack_start_ptr = &stack_start;
  stack_limit_ptr = STACK_LIMIT (stack_start_ptr, stack_size);
  k = setjmp (errjmp);
  if (k == 2)
    return (2);
  if (want_sigint)
    signal (SIGINT, handle_sigint);
  signal (SIGFPE, handle_sigfpe);
  catch_framep = (struct catch_frame *) NULL;
  errjmp_ok = 1;
  interrupt_differed = 0;
  nointerrupt = 0;
  if (want_init && init_file && (k == 0))
    vload (init_file, 0, 1);
  if (!h)
    {
      hd.repl_puts = repl_puts;
      hd.repl_read = repl_read;
      hd.repl_eval = repl_eval;
      hd.repl_print = repl_print;
      return (repl (&hd));
    }
  else
    return (repl (h));
}

static void
ignore_puts (char *st)
{
}

static void
noprompt_puts (char *st)
{
  if (strcmp (st, "> ") != 0)
    put_st (st);
}

static char *repl_c_string_arg = NULL;
static long repl_c_string_flag = 0;

static LISP
repl_c_string_read (void)
{
  LISP s;
  if (repl_c_string_arg == NULL)
    return (get_eof_val ());
  s = strcons (strlen (repl_c_string_arg), repl_c_string_arg);
  repl_c_string_arg = NULL;
  return (read_from_string (s));
}

static void
ignore_print (LISP x)
{
  repl_c_string_flag = 1;
}

static void
not_ignore_print (LISP x)
{
  repl_c_string_flag = 1;
  lprint (x, NIL);
}

long
repl_c_string (char *str,
	       long want_sigint, long want_init, long want_print)
{
  struct repl_hooks h;
  long retval;
  if (want_print)
    h.repl_puts = noprompt_puts;
  else
    h.repl_puts = ignore_puts;
  h.repl_read = repl_c_string_read;
  h.repl_eval = NULL;
  if (want_print)
    h.repl_print = not_ignore_print;
  else
    h.repl_print = ignore_print;
  repl_c_string_arg = str;
  repl_c_string_flag = 0;
  retval = repl_driver (want_sigint, want_init, &h);
  if (retval != 0)
    return (retval);
  else if (repl_c_string_flag == 1)
    return (0);
  else
    return (2);
}

double
myruntime (void)
{
  double total;
  struct tms b;
  times (&b);
  total = b.tms_utime;
  total += b.tms_stime;
  return (total / 60.0);
}

#if defined(__osf__)
#include <sys/timers.h>
#ifndef TIMEOFDAY
#define TIMEOFDAY 1
#endif
double
myrealtime (void)
{
  struct timespec x;
  if (!getclock (TIMEOFDAY, &x))
    return (x.tv_sec + (((double) x.tv_nsec) * 1.0e-9));
  else
    return (0.0);
}
#endif

#if defined(VMS)
#include <ssdef.h>
#include <starlet.h>

double
myrealtime (void)
{
  unsigned long x[2];
  static double c = 0.0;
  if (sys$gettim (&x) == SS$_NORMAL)
    {
      if (c == 0.0)
	c = pow ((double) 2, (double) 31) * 100.0e-9;
      return (x[0] * 100.0e-9 + x[1] * c);
    }
  else
    return (0.0);
}

#endif

#if !defined(__osf__) & !defined(VMS)
double
myrealtime (void)
{
  time_t x;
  time (&x);
  return ((double) x);
}
#endif

void
set_repl_hooks (void (*puts_f) (char *),
		LISP (*read_f) (void),
		LISP (*eval_f) (LISP),
		void (*print_f) (LISP))
{
  repl_puts = puts_f;
  repl_read = read_f;
  repl_eval = eval_f;
  repl_print = print_f;
}

void
gput_st (struct gen_printio *f, char *st)
{
  PUTS_FCN (st, f);
}

void
fput_st (FILE * f, char *st)
{
  long flag;
  flag = no_interrupt (1);
  if (siod_verbose_level >= 1)
    {
      fprintf (f, "%s", st);
      fflush (siod_output);
    }
  no_interrupt (flag);
}

int
fputs_fcn (char *st, void *cb)
{
  fput_st ((FILE *) cb, st);
  return (1);
}

void
put_st (char *st)
{
  fput_st (siod_output, st);
  fflush (siod_output);
}

void
grepl_puts (char *st, void (*repl_puts) (char *))
{
  if (repl_puts == NULL)
    put_st (st);
  else
    (*repl_puts) (st);
}

long
repl (struct repl_hooks *h)
{
  LISP x, cw = 0;
  double rt, ct;
  while (1)
    {
      if ((gc_kind_copying == 1) && ((gc_status_flag) || heap >= heap_end))
	{
	  rt = myruntime ();
	  gc_stop_and_copy ();
	  if (siod_verbose_level >= 2)
	    {
	      sprintf (tkbuffer,
		       "GC took %g seconds, %ld compressed to %d, %d free\n",
		       myruntime () - rt, old_heap_used, heap - heap_org, heap_end - heap);
	      grepl_puts (tkbuffer, h->repl_puts);
	    }
	}
      if (siod_verbose_level >= 2)
	grepl_puts ("> ", h->repl_puts);
      if (h->repl_read == NULL)
	x = lread (NIL);
      else
	x = (*h->repl_read) ();
      if EQ
	(x, eof_val) break;

      rt = myruntime ();
      ct = myrealtime ();
      if (gc_kind_copying == 1)
	cw = heap;
      else
	{
	  gc_cells_allocated = 0;
	  gc_time_taken = 0.0;
	}
      if (h->repl_eval == NULL)
	repl_return_val = x = leval (x, NIL);
      else
	repl_return_val = x = (*h->repl_eval) (x);
      if (gc_kind_copying == 1)
	sprintf (tkbuffer,
		 "Evaluation took %g seconds %d cons work, %g real.\n",
		 myruntime () - rt,
		 heap - cw,
		 myrealtime () - ct);
      else
	sprintf (tkbuffer,
	  "Evaluation took %g seconds (%g in gc) %ld cons work, %g real.\n",
		 myruntime () - rt,
		 gc_time_taken,
		 gc_cells_allocated,
		 myrealtime () - ct);
      if (siod_verbose_level >= 3)
	grepl_puts (tkbuffer, h->repl_puts);
      if (h->repl_print == NULL)
	{
	  if (siod_verbose_level >= 2)
	    lprint (x, NIL);
	}
      else
	(*h->repl_print) (x);
    }

  return (0);
}

void
set_fatal_exit_hook (void (*fcn) (void))
{
  fatal_exit_hook = fcn;
}

static long inside_err = 0;

LISP
err (char *message, LISP x)
{
  struct catch_frame *l;
  long was_inside = inside_err;
  LISP retval, nx;
  char *msg, *eobj;
  nointerrupt = 1;
  if ((!message) && CONSP (x) && TYPEP (CAR (x), tc_string))
    {
      msg = get_c_string (CAR (x));
      nx = CDR (x);
      retval = x;
    }
  else
    {
      msg = message;
      nx = x;
      retval = NIL;
    }
  if ((eobj = try_get_c_string (nx)) && !memchr (eobj, 0, 30))
    eobj = NULL;

  if NULLP
    (nx)
      sprintf (siod_err_msg, "ERROR: %s\n", msg);
  else if (eobj)
    sprintf (siod_err_msg, "ERROR: %s (errobj %s)\n", msg, eobj);
  else
    sprintf (siod_err_msg, "ERROR: %s (see errobj)\n", msg);

  if ((siod_verbose_level >= 1) && msg)
    {
      fprintf (siod_output, "%s\n", siod_err_msg);
      fflush (siod_output);
    }
  if (errjmp_ok == 1)
    {
      inside_err = 1;
      setvar (sym_errobj, nx, NIL);
      for (l = catch_framep; l; l = (*l).next)
	if (EQ ((*l).tag, sym_errobj) ||
	    EQ ((*l).tag, sym_catchall))
	  {
	    if (!msg)
	      msg = "quit";
	    (*l).retval = (NNULLP (retval) ? retval :
			   (was_inside) ? NIL :
			   cons (strcons (strlen (msg), msg), nx));
	    nointerrupt = 0;
	    inside_err = 0;
	    longjmp ((*l).cframe, 2);
	  }
      inside_err = 0;
      longjmp (errjmp, (msg) ? 1 : 2);
    }
  if (siod_verbose_level >= 1)
    {
      fprintf (stderr, "FATAL ERROR DURING STARTUP OR CRITICAL CODE SECTION\n");
      fflush (stderr);
    }
  if (fatal_exit_hook)
    (*fatal_exit_hook) ();
  else
    exit (1);
  return (NIL);
}

LISP
errswitch (void)
{
  return (err ("BUG. Reached impossible case", NIL));
}

void
err_stack (char *ptr)
     /* The user could be given an option to continue here */
{
  err ("the currently assigned stack limit has been exceded", NIL);
}

LISP
stack_limit (LISP amount, LISP silent)
{
  if NNULLP
    (amount)
    {
      stack_size = get_c_long (amount);
      stack_limit_ptr = STACK_LIMIT (stack_start_ptr, stack_size);
    }
  if NULLP
    (silent)
    {
      sprintf (tkbuffer, "Stack_size = %ld bytes, [%p,%p]\n",
	       stack_size, stack_start_ptr, stack_limit_ptr);
      put_st (tkbuffer);
      return (NIL);
    }
  else
    return (flocons (stack_size));
}

char *
try_get_c_string (LISP x)
{
  if TYPEP
    (x, tc_symbol)
      return (PNAME (x));
  else if TYPEP
    (x, tc_string)
      return (x->storage_as.string.data);
  else
    return (NULL);
}

char *
get_c_string (LISP x)
{
  if TYPEP
    (x, tc_symbol)
      return (PNAME (x));
  else if TYPEP
    (x, tc_string)
      return (x->storage_as.string.data);
  else
    err ("not a symbol or string", x);
  return (NULL);
}

char *
get_c_string_dim (LISP x, long *len)
{
  switch (TYPE (x))
    {
    case tc_symbol:
      *len = strlen (PNAME (x));
      return (PNAME (x));
    case tc_string:
    case tc_byte_array:
      *len = x->storage_as.string.dim;
      return (x->storage_as.string.data);
    case tc_long_array:
      *len = x->storage_as.long_array.dim * sizeof (long);
      return ((char *) x->storage_as.long_array.data);
    default:
      err ("not a symbol or string", x);
      return (NULL);
    }
}

LISP
lerr (LISP message, LISP x)
{
  if (CONSP (message) && TYPEP (CAR (message), tc_string))
    err (NULL, message);
  else
    err (get_c_string (message), x);
  return (NIL);
}

void
gc_fatal_error (void)
{
  err ("ran out of storage", NIL);
}

LISP
newcell (long type)
{
  LISP z;
  NEWCELL (z, type);
  return (z);
}

LISP
cons (LISP x, LISP y)
{
  LISP z;
  NEWCELL (z, tc_cons);
  CAR (z) = x;
  CDR (z) = y;
  return (z);
}

LISP
consp (LISP x)
{
  if CONSP
    (x) return (sym_t);
  else
    return (NIL);
}

LISP
car (LISP x)
{
  switch TYPE
    (x)
    {
    case tc_nil:
      return (NIL);
    case tc_cons:
      return (CAR (x));
    default:
      return (err ("wta to car", x));
    }
}

LISP
cdr (LISP x)
{
  switch TYPE
    (x)
    {
    case tc_nil:
      return (NIL);
    case tc_cons:
      return (CDR (x));
    default:
      return (err ("wta to cdr", x));
    }
}

LISP
setcar (LISP cell, LISP value)
{
  if NCONSP
    (cell) err ("wta to setcar", cell);
  return (CAR (cell) = value);
}

LISP
setcdr (LISP cell, LISP value)
{
  if NCONSP
    (cell) err ("wta to setcdr", cell);
  return (CDR (cell) = value);
}

LISP
flocons (double x)
{
  LISP z;
  long n;
  if ((inums_dim > 0) &&
      ((x - (n = (long) x)) == 0) &&
      (x >= 0) &&
      (n < inums_dim))
    return (inums[n]);
  NEWCELL (z, tc_flonum);
  FLONM (z) = x;
  return (z);
}

LISP
numberp (LISP x)
{
  if FLONUMP
    (x) return (sym_t);
  else
    return (NIL);
}

LISP
plus (LISP x, LISP y)
{
  if NULLP
    (y)
      return (NULLP (x) ? flocons (0) : x);
  if NFLONUMP
    (x) err ("wta(1st) to plus", x);
  if NFLONUMP
    (y) err ("wta(2nd) to plus", y);
  return (flocons (FLONM (x) + FLONM (y)));
}

LISP
ltimes (LISP x, LISP y)
{
  if NULLP
    (y)
      return (NULLP (x) ? flocons (1) : x);
  if NFLONUMP
    (x) err ("wta(1st) to times", x);
  if NFLONUMP
    (y) err ("wta(2nd) to times", y);
  return (flocons (FLONM (x) * FLONM (y)));
}

LISP
difference (LISP x, LISP y)
{
  if NFLONUMP
    (x) err ("wta(1st) to difference", x);
  if NULLP
    (y)
      return (flocons (-FLONM (x)));
  else
    {
      if NFLONUMP
	(y) err ("wta(2nd) to difference", y);
      return (flocons (FLONM (x) - FLONM (y)));
    }
}

LISP
Quotient (LISP x, LISP y)
{
  if NFLONUMP
    (x) err ("wta(1st) to quotient", x);
  if NULLP
    (y)
      return (flocons (1 / FLONM (x)));
  else
    {
      if NFLONUMP
	(y) err ("wta(2nd) to quotient", y);
      return (flocons (FLONM (x) / FLONM (y)));
    }
}

LISP
lllabs (LISP x)
{
  double v;
  if NFLONUMP
    (x) err ("wta to abs", x);
  v = FLONM (x);
  if (v < 0)
    return (flocons (-v));
  else
    return (x);
}

LISP
lsqrt (LISP x)
{
  if NFLONUMP
    (x) err ("wta to sqrt", x);
  return (flocons (sqrt (FLONM (x))));
}

LISP
greaterp (LISP x, LISP y)
{
  if NFLONUMP
    (x) err ("wta(1st) to greaterp", x);
  if NFLONUMP
    (y) err ("wta(2nd) to greaterp", y);
  if (FLONM (x) > FLONM (y))
    return (sym_t);
  return (NIL);
}

LISP
lessp (LISP x, LISP y)
{
  if NFLONUMP
    (x) err ("wta(1st) to lessp", x);
  if NFLONUMP
    (y) err ("wta(2nd) to lessp", y);
  if (FLONM (x) < FLONM (y))
    return (sym_t);
  return (NIL);
}

LISP
greaterEp (LISP x, LISP y)
{
  if NFLONUMP
    (x) err ("wta(1st) to greaterp", x);
  if NFLONUMP
    (y) err ("wta(2nd) to greaterp", y);
  if (FLONM (x) >= FLONM (y))
    return (sym_t);
  return (NIL);
}

LISP
lessEp (LISP x, LISP y)
{
  if NFLONUMP
    (x) err ("wta(1st) to lessp", x);
  if NFLONUMP
    (y) err ("wta(2nd) to lessp", y);
  if (FLONM (x) <= FLONM (y))
    return (sym_t);
  return (NIL);
}

LISP
lmax (LISP x, LISP y)
{
  if NULLP
    (y) return (x);
  if NFLONUMP
    (x) err ("wta(1st) to max", x);
  if NFLONUMP
    (y) err ("wta(2nd) to max", y);
  return ((FLONM (x) > FLONM (y)) ? x : y);
}

LISP
lmin (LISP x, LISP y)
{
  if NULLP
    (y) return (x);
  if NFLONUMP
    (x) err ("wta(1st) to min", x);
  if NFLONUMP
    (y) err ("wta(2nd) to min", y);
  return ((FLONM (x) < FLONM (y)) ? x : y);
}

LISP
eq (LISP x, LISP y)
{
  if EQ
    (x, y) return (sym_t);
  else
    return (NIL);
}

LISP
eql (LISP x, LISP y)
{
  if EQ
    (x, y) return (sym_t);
  else if NFLONUMP
    (x) return (NIL);
  else if NFLONUMP
    (y) return (NIL);
  else if (FLONM (x) == FLONM (y))
    return (sym_t);
  return (NIL);
}

LISP
symcons (char *pname, LISP vcell)
{
  LISP z;
  NEWCELL (z, tc_symbol);
  PNAME (z) = pname;
  VCELL (z) = vcell;
  return (z);
}

LISP
symbolp (LISP x)
{
  if SYMBOLP
    (x) return (sym_t);
  else
    return (NIL);
}

LISP
err_ubv (LISP v)
{
  return (err ("unbound variable", v));
}

LISP
symbol_boundp (LISP x, LISP env)
{
  LISP tmp;
  if NSYMBOLP
    (x) err ("not a symbol", x);
  tmp = envlookup (x, env);
  if NNULLP
    (tmp) return (sym_t);
  if EQ
    (VCELL (x), unbound_marker) return (NIL);
  else
    return (sym_t);
}

LISP
symbol_value (LISP x, LISP env)
{
  LISP tmp;
  if NSYMBOLP
    (x) err ("not a symbol", x);
  tmp = envlookup (x, env);
  if NNULLP
    (tmp) return (CAR (tmp));
  tmp = VCELL (x);
  if EQ
    (tmp, unbound_marker) err_ubv (x);
  return (tmp);
}



char *
must_malloc (unsigned long size)
{
  char *tmp;
  tmp = (char *) malloc ((size) ? size : 1);
  if (tmp == (char *) NULL)
    err ("failed to allocate storage from system", NIL);
  return (tmp);
}

LISP
gen_intern (char *name, long copyp)
{
  LISP l, sym, sl;
  char *cname;
  long hash = 0, n, c, flag;
  flag = no_interrupt (1);
  if (obarray_dim > 1)
    {
      hash = 0;
      n = obarray_dim;
      cname = name;
      while ((c = *cname++))
	hash = ((hash * 17) ^ c) % n;
      sl = obarray[hash];
    }
  else
    sl = oblistvar;
  for (l = sl; NNULLP (l); l = CDR (l))
    if (strcmp (name, PNAME (CAR (l))) == 0)
      {
	no_interrupt (flag);
	return (CAR (l));
      }
  if (copyp == 1)
    {
      cname = (char *) must_malloc (strlen (name) + 1);
      strcpy (cname, name);
    }
  else
    cname = name;
  sym = symcons (cname, unbound_marker);
  if (obarray_dim > 1)
    obarray[hash] = cons (sym, sl);
  oblistvar = cons (sym, oblistvar);
  no_interrupt (flag);
  return (sym);
}

LISP
cintern (char *name)
{
  return (gen_intern (name, 0));
}

LISP
rintern (char *name)
{
  return (gen_intern (name, 1));
}

LISP
intern (LISP name)
{
  return (rintern (get_c_string (name)));
}

LISP
subrcons (long type, char *name, SUBR_FUNC f)
{
  LISP z;
  NEWCELL (z, type);
  (*z).storage_as.subr.name = name;
  (*z).storage_as.subr0.f = f;
  return (z);
}

LISP
closure (LISP env, LISP code)
{
  LISP z;
  NEWCELL (z, tc_closure);
  (*z).storage_as.closure.env = env;
  (*z).storage_as.closure.code = code;
  return (z);
}

void
gc_protect (LISP * location)
{
  gc_protect_n (location, 1);
}

void
gc_protect_n (LISP * location, long n)
{
  struct gc_protected *reg;
  reg = (struct gc_protected *) must_malloc (sizeof (struct gc_protected));
  (*reg).location = location;
  (*reg).length = n;
  (*reg).next = protected_registers;
  protected_registers = reg;
}

void
gc_protect_sym (LISP * location, char *st)
{
  *location = cintern (st);
  gc_protect (location);
}

void
gc_unprotect (LISP * location)
{
  struct gc_protected *reg;
  struct gc_protected *prev_reg;

  prev_reg = NULL;
  reg = protected_registers;

  while (reg)
    {
      if (location == reg->location)
	{
	  if (prev_reg)
	    prev_reg->next = reg->next;
	  if (reg == protected_registers)
	    protected_registers = protected_registers->next;

	  free (reg);
	  break;
	}

      prev_reg = reg;
      reg = reg->next;
    }
}

void
scan_registers (void)
{
  struct gc_protected *reg;
  LISP *location;
  long j, n;

  for (reg = protected_registers; reg; reg = (*reg).next)
    {
      location = (*reg).location;
      n = (*reg).length;
      for (j = 0; j < n; ++j)
	location[j] = gc_relocate (location[j]);
    }
}

void
init_storage (void)
{
  long j;
  LISP stack_start;
  if (stack_start_ptr == NULL)
    stack_start_ptr = &stack_start;
  init_storage_1 ();
  init_storage_a ();
  set_gc_hooks (tc_c_file, 0, 0, 0, file_gc_free, &j);
  set_print_hooks (tc_c_file, file_prin1);
}

void
init_storage_1 (void)
{
  LISP ptr;
  long j;
  tkbuffer = (char *) must_malloc (TKBUFFERN + 1);
  if (((gc_kind_copying == 1) && (nheaps != 2)) || (nheaps < 1))
    err ("invalid number of heaps", NIL);
  heaps = (LISP *) must_malloc (sizeof (LISP) * nheaps);
  for (j = 0; j < nheaps; ++j)
    heaps[j] = NULL;
  heaps[0] = (LISP) must_malloc (sizeof (struct obj) * heap_size);
  heap = heaps[0];
  heap_org = heap;
  heap_end = heap + heap_size;
  if (gc_kind_copying == 1)
    heaps[1] = (LISP) must_malloc (sizeof (struct obj) * heap_size);
  else
    freelist = NIL;
  gc_protect (&oblistvar);
  if (obarray_dim > 1)
    {
      obarray = (LISP *) must_malloc (sizeof (LISP) * obarray_dim);
      for (j = 0; j < obarray_dim; ++j)
	obarray[j] = NIL;
      gc_protect_n (obarray, obarray_dim);
    }
  unbound_marker = cons (cintern ("**unbound-marker**"), NIL);
  gc_protect (&unbound_marker);
  eof_val = cons (cintern ("eof"), NIL);
  gc_protect (&eof_val);
  gc_protect_sym (&sym_t, "t");
  setvar (sym_t, sym_t, NIL);
  setvar (cintern ("nil"), NIL, NIL);
  setvar (cintern ("let"), cintern ("let-internal-macro"), NIL);
  setvar (cintern ("let*"), cintern ("let*-macro"), NIL);
  setvar (cintern ("letrec"), cintern ("letrec-macro"), NIL);
  gc_protect_sym (&sym_errobj, "errobj");
  setvar (sym_errobj, NIL, NIL);
  gc_protect_sym (&sym_catchall, "all");
  gc_protect_sym (&sym_progn, "begin");
  gc_protect_sym (&sym_lambda, "lambda");
  gc_protect_sym (&sym_quote, "quote");
  gc_protect_sym (&sym_dot, ".");
  gc_protect_sym (&sym_after_gc, "*after-gc*");
  setvar (sym_after_gc, NIL, NIL);
  gc_protect_sym (&sym_eval_history_ptr, "*eval-history-ptr*");
  setvar (sym_eval_history_ptr, NIL, NIL);
  if (inums_dim > 0)
    {
      inums = (LISP *) must_malloc (sizeof (LISP) * inums_dim);
      for (j = 0; j < inums_dim; ++j)
	{
	  NEWCELL (ptr, tc_flonum);
	  FLONM (ptr) = j;
	  inums[j] = ptr;
	}
      gc_protect_n (inums, inums_dim);
    }
}

void
init_subr (char *name, long type, SUBR_FUNC fcn)
{
  setvar (cintern (name), subrcons (type, name, fcn), NIL);
}

void
init_subr_0 (char *name, LISP (*fcn) (void))
{
  init_subr (name, tc_subr_0, (SUBR_FUNC) fcn);
}

void
init_subr_1 (char *name, LISP (*fcn) (LISP))
{
  init_subr (name, tc_subr_1, (SUBR_FUNC) fcn);
}

void
init_subr_2 (char *name, LISP (*fcn) (LISP, LISP))
{
  init_subr (name, tc_subr_2, (SUBR_FUNC) fcn);
}

void
init_subr_2n (char *name, LISP (*fcn) (LISP, LISP))
{
  init_subr (name, tc_subr_2n, (SUBR_FUNC) fcn);
}

void
init_subr_3 (char *name, LISP (*fcn) (LISP, LISP, LISP))
{
  init_subr (name, tc_subr_3, (SUBR_FUNC) fcn);
}

void
init_subr_4 (char *name, LISP (*fcn) (LISP, LISP, LISP, LISP))
{
  init_subr (name, tc_subr_4, (SUBR_FUNC) fcn);
}

void
init_subr_5 (char *name, LISP (*fcn) (LISP, LISP, LISP, LISP, LISP))
{
  init_subr (name, tc_subr_5, (SUBR_FUNC) fcn);
}

void
init_lsubr (char *name, LISP (*fcn) (LISP))
{
  init_subr (name, tc_lsubr, (SUBR_FUNC) fcn);
}

void
init_fsubr (char *name, LISP (*fcn) (LISP, LISP))
{
  init_subr (name, tc_fsubr, (SUBR_FUNC) fcn);
}

void
init_msubr (char *name, LISP (*fcn) (LISP *, LISP *))
{
  init_subr (name, tc_msubr, (SUBR_FUNC) fcn);
}

LISP
assq (LISP x, LISP alist)
{
  LISP l, tmp;
  for (l = alist; CONSP (l); l = CDR (l))
    {
      tmp = CAR (l);
      if (CONSP (tmp) && EQ (CAR (tmp), x))
	return (tmp);
      INTERRUPT_CHECK ();
    }
  if EQ
    (l, NIL) return (NIL);
  return (err ("improper list to assq", alist));
}


struct user_type_hooks *
get_user_type_hooks (long type)
{
  long n;
  if (user_types == NULL)
    {
      n = sizeof (struct user_type_hooks) * tc_table_dim;
      user_types = (struct user_type_hooks *) must_malloc (n);
      memset (user_types, 0, n);
    }
  if ((type >= 0) && (type < tc_table_dim))
    return (&user_types[type]);
  else
    err ("type number out of range", NIL);
  return (NULL);
}

long
allocate_user_tc (void)
{
  long x = user_tc_next;
  if (x > tc_user_max)
    err ("ran out of user type codes", NIL);
  ++user_tc_next;
  return (x);
}

void
set_gc_hooks (long type,
	      LISP (*rel) (LISP),
	      LISP (*mark) (LISP),
	      void (*scan) (LISP),
	      void (*free) (LISP),
	      long *kind)
{
  struct user_type_hooks *p;
  p = get_user_type_hooks (type);
  p->gc_relocate = rel;
  p->gc_scan = scan;
  p->gc_mark = mark;
  p->gc_free = free;
  *kind = gc_kind_copying;
}

LISP
gc_relocate (LISP x)
{
  LISP nw;
  struct user_type_hooks *p;
  if EQ
    (x, NIL) return (NIL);
  if ((*x).gc_mark == 1)
    return (CAR (x));
  switch TYPE
    (x)
    {
    case tc_flonum:
    case tc_cons:
    case tc_symbol:
    case tc_closure:
    case tc_subr_0:
    case tc_subr_1:
    case tc_subr_2:
    case tc_subr_2n:
    case tc_subr_3:
    case tc_subr_4:
    case tc_subr_5:
    case tc_lsubr:
    case tc_fsubr:
    case tc_msubr:
      if ((nw = heap) >= heap_end)
	gc_fatal_error ();
      heap = nw + 1;
      memcpy (nw, x, sizeof (struct obj));
      break;
    default:
      p = get_user_type_hooks (TYPE (x));
      if (p->gc_relocate)
	nw = (*p->gc_relocate) (x);
      else
	{
	  if ((nw = heap) >= heap_end)
	    gc_fatal_error ();
	  heap = nw + 1;
	  memcpy (nw, x, sizeof (struct obj));
	}
    }
  (*x).gc_mark = 1;
  CAR (x) = nw;
  return (nw);
}

LISP
get_newspace (void)
{
  LISP newspace;
  if (heap_org == heaps[0])
    newspace = heaps[1];
  else
    newspace = heaps[0];
  heap = newspace;
  heap_org = heap;
  heap_end = heap + heap_size;
  return (newspace);
}

void
scan_newspace (LISP newspace)
{
  LISP ptr;
  struct user_type_hooks *p;
  for (ptr = newspace; ptr < heap; ++ptr)
    {
      switch TYPE
	(ptr)
	{
	case tc_cons:
	case tc_closure:
	  CAR (ptr) = gc_relocate (CAR (ptr));
	  CDR (ptr) = gc_relocate (CDR (ptr));
	  break;
	case tc_symbol:
	  VCELL (ptr) = gc_relocate (VCELL (ptr));
	  break;
	case tc_flonum:
	case tc_subr_0:
	case tc_subr_1:
	case tc_subr_2:
	case tc_subr_2n:
	case tc_subr_3:
	case tc_subr_4:
	case tc_subr_5:
	case tc_lsubr:
	case tc_fsubr:
	case tc_msubr:
	  break;
	default:
	  p = get_user_type_hooks (TYPE (ptr));
	  if (p->gc_scan)
	    (*p->gc_scan) (ptr);
	}
    }
}

void
free_oldspace (LISP space, LISP end)
{
  LISP ptr;
  struct user_type_hooks *p;
  for (ptr = space; ptr < end; ++ptr)
    if (ptr->gc_mark == 0)
      switch TYPE
	(ptr)
	{
	case tc_cons:
	case tc_closure:
	case tc_symbol:
	case tc_flonum:
	case tc_subr_0:
	case tc_subr_1:
	case tc_subr_2:
	case tc_subr_2n:
	case tc_subr_3:
	case tc_subr_4:
	case tc_subr_5:
	case tc_lsubr:
	case tc_fsubr:
	case tc_msubr:
	  break;
	default:
	  p = get_user_type_hooks (TYPE (ptr));
	  if (p->gc_free)
	    (*p->gc_free) (ptr);
	}
}

void
gc_stop_and_copy (void)
{
  LISP newspace, oldspace, end;
  long flag;
  flag = no_interrupt (1);
  errjmp_ok = 0;
  oldspace = heap_org;
  end = heap;
  old_heap_used = end - oldspace;
  newspace = get_newspace ();
  scan_registers ();
  scan_newspace (newspace);
  free_oldspace (oldspace, end);
  errjmp_ok = 1;
  no_interrupt (flag);
}

LISP
allocate_aheap (void)
{
  long j, flag;
  LISP ptr, end, next;
  gc_kind_check ();
  for (j = 0; j < nheaps; ++j)
    if (!heaps[j])
      {
	flag = no_interrupt (1);
	if (gc_status_flag && (siod_verbose_level >= 4))
	  fprintf (siod_output, "[allocating heap %ld]\n", j);
	heaps[j] = (LISP) must_malloc (sizeof (struct obj) * heap_size);
	ptr = heaps[j];
	end = heaps[j] + heap_size;
	while (1)
	  {
	    (*ptr).type = tc_free_cell;
	    next = ptr + 1;
	    if (next < end)
	      {
		CDR (ptr) = next;
		ptr = next;
	      }
	    else
	      {
		CDR (ptr) = freelist;
		break;
	      }
	  }
	freelist = heaps[j];
	flag = no_interrupt (flag);
	return (sym_t);
      }
  return (NIL);
}

void
gc_for_newcell (void)
{
  long flag, n;
  LISP l;
  if (heap < heap_end)
    {
      freelist = heap;
      CDR (freelist) = NIL;
      ++heap;
      return;
    }
  if (errjmp_ok == 0)
    gc_fatal_error ();
  flag = no_interrupt (1);
  errjmp_ok = 0;
  gc_mark_and_sweep ();
  errjmp_ok = 1;
  no_interrupt (flag);
  for (n = 0, l = freelist; (n < 100) && NNULLP (l); ++n)
    l = CDR (l);
  if (n == 0)
    {
      if NULLP
	(allocate_aheap ())
	  gc_fatal_error ();
    }
  else if ((n == 100) && NNULLP (sym_after_gc))
    leval (leval (sym_after_gc, NIL), NIL);
  else
    allocate_aheap ();
}

void
gc_mark_and_sweep (void)
{
  LISP stack_end;
  gc_ms_stats_start ();
  while (heap < heap_end)
    {
      heap->type = tc_free_cell;
      heap->gc_mark = 0;
      ++heap;
    }
  setjmp (save_regs_gc_mark);
  mark_locations ((LISP *) save_regs_gc_mark,
      (LISP *) (((char *) save_regs_gc_mark) + sizeof (save_regs_gc_mark)));
  mark_protected_registers ();
  mark_locations ((LISP *) stack_start_ptr,
		  (LISP *) & stack_end);
#ifdef THINK_C
  mark_locations ((LISP *) ((char *) stack_start_ptr + 2),
		  (LISP *) ((char *) &stack_end + 2));
#endif
  gc_sweep ();
  gc_ms_stats_end ();
}

void
gc_ms_stats_start (void)
{
  gc_rt = myruntime ();
  gc_cells_collected = 0;
  if (gc_status_flag && (siod_verbose_level >= 4))
    fprintf (siod_output, "[starting GC]\n");
}

void
gc_ms_stats_end (void)
{
  gc_rt = myruntime () - gc_rt;
  gc_time_taken = gc_time_taken + gc_rt;
  if (gc_status_flag && (siod_verbose_level >= 4))
    fprintf (siod_output, "[GC took %g cpu seconds, %ld cells collected]\n",
	     gc_rt,
	     gc_cells_collected);
}

void
gc_mark (LISP ptr)
{
  struct user_type_hooks *p;
gc_mark_loop:
  if NULLP
    (ptr) return;
  if ((*ptr).gc_mark)
    return;
  (*ptr).gc_mark = 1;
  switch ((*ptr).type)
    {
    case tc_flonum:
      break;
    case tc_cons:
      gc_mark (CAR (ptr));
      ptr = CDR (ptr);
      goto gc_mark_loop;
    case tc_symbol:
      ptr = VCELL (ptr);
      goto gc_mark_loop;
    case tc_closure:
      gc_mark ((*ptr).storage_as.closure.code);
      ptr = (*ptr).storage_as.closure.env;
      goto gc_mark_loop;
    case tc_subr_0:
    case tc_subr_1:
    case tc_subr_2:
    case tc_subr_2n:
    case tc_subr_3:
    case tc_subr_4:
    case tc_subr_5:
    case tc_lsubr:
    case tc_fsubr:
    case tc_msubr:
      break;
    default:
      p = get_user_type_hooks (TYPE (ptr));
      if (p->gc_mark)
	ptr = (*p->gc_mark) (ptr);
    }
}

void
mark_protected_registers (void)
{
  struct gc_protected *reg;
  LISP *location;
  long j, n;
  for (reg = protected_registers; reg; reg = (*reg).next)
    {
      location = (*reg).location;
      n = (*reg).length;
      for (j = 0; j < n; ++j)
	gc_mark (location[j]);
    }
}

void
mark_locations (LISP * start, LISP * end)
{
  LISP *tmp;
  long n;
  if (start > end)
    {
      tmp = start;
      start = end;
      end = tmp;
    }
  n = end - start;
  mark_locations_array (start, n);
}

long
looks_pointerp (LISP p)
{
  long j;
  LISP h;
  for (j = 0; j < nheaps; ++j)
    if ((h = heaps[j]) &&
	(p >= h) &&
	(p < (h + heap_size)) &&
	(((((char *) p) - ((char *) h)) % sizeof (struct obj)) == 0) &&
	NTYPEP (p, tc_free_cell))
        return (1);
  return (0);
}

void
mark_locations_array (LISP * x, long n)
{
  int j;
  LISP p;
  for (j = 0; j < n; ++j)
    {
      p = x[j];
      if (looks_pointerp (p))
	gc_mark (p);
    }
}

void
gc_sweep (void)
{
  LISP ptr, end, nfreelist, org;
  long n, k;
  struct user_type_hooks *p;
  end = heap_end;
  n = 0;
  nfreelist = NIL;
  for (k = 0; k < nheaps; ++k)
    if (heaps[k])
      {
	org = heaps[k];
	end = org + heap_size;
	for (ptr = org; ptr < end; ++ptr)
	  if (((*ptr).gc_mark == 0))
	    {
	      switch ((*ptr).type)
		{
		case tc_free_cell:
		case tc_cons:
		case tc_closure:
		case tc_symbol:
		case tc_flonum:
		case tc_subr_0:
		case tc_subr_1:
		case tc_subr_2:
		case tc_subr_2n:
		case tc_subr_3:
		case tc_subr_4:
		case tc_subr_5:
		case tc_lsubr:
		case tc_fsubr:
		case tc_msubr:
		  break;
		default:
		  p = get_user_type_hooks (TYPE (ptr));
		  if (p->gc_free)
		    (*p->gc_free) (ptr);
		}
	      ++n;
	      (*ptr).type = tc_free_cell;
	      CDR (ptr) = nfreelist;
	      nfreelist = ptr;
	    }
	  else
	    (*ptr).gc_mark = 0;
      }
  gc_cells_collected = n;
  freelist = nfreelist;
}

void
gc_kind_check (void)
{
  if (gc_kind_copying == 1)
    err ("cannot perform operation with stop-and-copy GC mode. Use -g0\n",
	 NIL);
}

LISP
user_gc (LISP args)
{
  long old_status_flag, flag;
  gc_kind_check ();
  flag = no_interrupt (1);
  errjmp_ok = 0;
  old_status_flag = gc_status_flag;
  if NNULLP
    (args)
      if NULLP
      (car (args)) gc_status_flag = 0;
    else
      gc_status_flag = 1;
  gc_mark_and_sweep ();
  gc_status_flag = old_status_flag;
  errjmp_ok = 1;
  no_interrupt (flag);
  return (NIL);
}

long
nactive_heaps (void)
{
  long m;
  for (m = 0; (m < nheaps) && heaps[m]; ++m);
  return (m);
}

long
freelist_length (void)
{
  long n;
  LISP l;
  for (n = 0, l = freelist; NNULLP (l); ++n)
    l = CDR (l);
  n += (heap_end - heap);
  return (n);
}

LISP
gc_status (LISP args)
{
  long n, m;
  if NNULLP
    (args)
      if NULLP
      (car (args)) gc_status_flag = 0;
    else
      gc_status_flag = 1;
  if (gc_kind_copying == 1)
    {
      if (gc_status_flag)
	put_st ("garbage collection is on\n");
      else
	put_st ("garbage collection is off\n");
      sprintf (tkbuffer, "%d allocated %d free\n",
	       heap - heap_org, heap_end - heap);
      put_st (tkbuffer);
    }
  else
    {
      if (gc_status_flag)
	put_st ("garbage collection verbose\n");
      else
	put_st ("garbage collection silent\n");
      {
	m = nactive_heaps ();
	n = freelist_length ();
	sprintf (tkbuffer, "%ld/%ld heaps, %ld allocated %ld free\n",
		 m, nheaps, m * heap_size - n, n);
	put_st (tkbuffer);
      }
    }
  return (NIL);
}

LISP
gc_info (LISP arg)
{
  switch (get_c_long (arg))
    {
    case 0:
      return ((gc_kind_copying == 1) ? sym_t : NIL);
    case 1:
      return (flocons (nactive_heaps ()));
    case 2:
      return (flocons (nheaps));
    case 3:
      return (flocons (heap_size));
    case 4:
      return (flocons ((gc_kind_copying == 1)
		       ? (long) (heap_end - heap)
		       : freelist_length ()));
    default:
      return (NIL);
    }
}

LISP
leval_args (LISP l, LISP env)
{
  LISP result, v1, v2, tmp;
  if NULLP
    (l) return (NIL);
  if NCONSP
    (l) err ("bad syntax argument list", l);
  result = cons (leval (CAR (l), env), NIL);
  for (v1 = result, v2 = CDR (l);
       CONSP (v2);
       v1 = tmp, v2 = CDR (v2))
    {
      tmp = cons (leval (CAR (v2), env), NIL);
      CDR (v1) = tmp;
    }
  if NNULLP
    (v2) err ("bad syntax argument list", l);
  return (result);
}

LISP
extend_env (LISP actuals, LISP formals, LISP env)
{
  if SYMBOLP
    (formals)
      return (cons (cons (cons (formals, NIL), cons (actuals, NIL)), env));
  return (cons (cons (formals, actuals), env));
}

#define ENVLOOKUP_TRICK 1

LISP
envlookup (LISP var, LISP env)
{
  LISP frame, al, fl, tmp;
  for (frame = env; CONSP (frame); frame = CDR (frame))
    {
      tmp = CAR (frame);
      if NCONSP
	(tmp) err ("damaged frame", tmp);
      for (fl = CAR (tmp), al = CDR (tmp); CONSP (fl); fl = CDR (fl), al = CDR (al))
	{
	  if NCONSP
	    (al) err ("too few arguments", tmp);
	  if EQ
	    (CAR (fl), var) return (al);
	}
      /* suggested by a user. It works for reference (although conses)
         but doesn't allow for set! to work properly... */
#if (ENVLOOKUP_TRICK)
      if (SYMBOLP (fl) && EQ (fl, var))
	return (cons (al, NIL));
#endif
    }
  if NNULLP
    (frame) err ("damaged env", env);
  return (NIL);
}

void
set_eval_hooks (long type, LISP (*fcn) (LISP, LISP *, LISP *))
{
  struct user_type_hooks *p;
  p = get_user_type_hooks (type);
  p->leval = fcn;
}

LISP
err_closure_code (LISP tmp)
{
  return (err ("closure code type not valid", tmp));
}

LISP
leval (LISP x, LISP env)
{
  LISP tmp, arg1;
  struct user_type_hooks *p;
  STACK_CHECK (&x);
loop:
  INTERRUPT_CHECK ();
  tmp = VCELL (sym_eval_history_ptr);
  if TYPEP
    (tmp, tc_cons)
    {
      CAR (tmp) = x;
      VCELL (sym_eval_history_ptr) = CDR (tmp);
    }
  switch TYPE
    (x)
    {
    case tc_symbol:
      tmp = envlookup (x, env);
      if NNULLP
	(tmp) return (CAR (tmp));
      tmp = VCELL (x);
      if EQ
	(tmp, unbound_marker) err_ubv (x);
      return (tmp);
    case tc_cons:
      tmp = CAR (x);
      switch TYPE
	(tmp)
	{
	case tc_symbol:
	  tmp = envlookup (tmp, env);
	  if NNULLP
	    (tmp)
	    {
	      tmp = CAR (tmp);
	      break;
	    }
	  tmp = VCELL (CAR (x));
	  if EQ
	    (tmp, unbound_marker) err_ubv (CAR (x));
	  break;
	case tc_cons:
	  tmp = leval (tmp, env);
	  break;
	}
      switch TYPE
	(tmp)
	{
	case tc_subr_0:
	  return (SUBR0 (tmp) ());
	case tc_subr_1:
	  return (SUBR1 (tmp) (leval (car (CDR (x)), env)));
	case tc_subr_2:
	  x = CDR (x);
	  arg1 = leval (car (x), env);
	  x = NULLP (x) ? NIL : CDR (x);
	  return (SUBR2 (tmp) (arg1,
			       leval (car (x), env)));
	case tc_subr_2n:
	  x = CDR (x);
	  arg1 = leval (car (x), env);
	  x = NULLP (x) ? NIL : CDR (x);
	  arg1 = SUBR2 (tmp) (arg1,
			      leval (car (x), env));
	  for (x = cdr (x); CONSP (x); x = CDR (x))
	    arg1 = SUBR2 (tmp) (arg1, leval (CAR (x), env));
	  return (arg1);
	case tc_subr_3:
	  x = CDR (x);
	  arg1 = leval (car (x), env);
	  x = NULLP (x) ? NIL : CDR (x);
	  return (SUBR3 (tmp) (arg1,
			       leval (car (x), env),
			       leval (car (cdr (x)), env)));

	case tc_subr_4:
	  x = CDR (x);
	  arg1 = leval (car (x), env);
	  x = NULLP (x) ? NIL : CDR (x);
	  return (SUBR4 (tmp) (arg1,
			       leval (car (x), env),
			       leval (car (cdr (x)), env),
			       leval (car (cdr (cdr (x))), env)));

	case tc_subr_5:
	  x = CDR (x);
	  arg1 = leval (car (x), env);
	  x = NULLP (x) ? NIL : CDR (x);
	  return (SUBR5 (tmp) (arg1,
			       leval (car (x), env),
			       leval (car (cdr (x)), env),
			       leval (car (cdr (cdr (x))), env),
			       leval (car (cdr (cdr (cdr (x)))), env)));

	case tc_lsubr:
	  return (SUBR1 (tmp) (leval_args (CDR (x), env)));
	case tc_fsubr:
	  return (SUBR2 (tmp) (CDR (x), env));
	case tc_msubr:
	  if NULLP
	    (SUBRM (tmp) (&x, &env)) return (x);
	  goto loop;
	case tc_closure:
	  switch TYPE
	    ((*tmp).storage_as.closure.code)
	    {
	    case tc_cons:
	      env = extend_env (leval_args (CDR (x), env),
				CAR ((*tmp).storage_as.closure.code),
				(*tmp).storage_as.closure.env);
	      x = CDR ((*tmp).storage_as.closure.code);
	      goto loop;
	    case tc_subr_1:
	      return (SUBR1 (tmp->storage_as.closure.code)
		      (tmp->storage_as.closure.env));
	    case tc_subr_2:
	      x = CDR (x);
	      arg1 = leval (car (x), env);
	      return (SUBR2 (tmp->storage_as.closure.code)
		      (tmp->storage_as.closure.env, arg1));
	    case tc_subr_3:
	      x = CDR (x);
	      arg1 = leval (car (x), env);
	      x = NULLP (x) ? NIL : CDR (x);
	      return (SUBR3 (tmp->storage_as.closure.code)
		      (tmp->storage_as.closure.env,
		       arg1,
		       leval (car (x), env)));
	    case tc_subr_4:
	      x = CDR (x);
	      arg1 = leval (car (x), env);
	      x = NULLP (x) ? NIL : CDR (x);
	      return (SUBR4 (tmp->storage_as.closure.code)
		      (tmp->storage_as.closure.env,
		       arg1,
		       leval (car (x), env),
		       leval (car (cdr (x)), env)));
	    case tc_subr_5:
	      x = CDR (x);
	      arg1 = leval (car (x), env);
	      x = NULLP (x) ? NIL : CDR (x);
	      return (SUBR5 (tmp->storage_as.closure.code)
		      (tmp->storage_as.closure.env,
		       arg1,
		       leval (car (x), env),
		       leval (car (cdr (x)), env),
		       leval (car (cdr (cdr (x))), env)));

	    case tc_lsubr:
	      return (SUBR1 (tmp->storage_as.closure.code)
		      (cons (tmp->storage_as.closure.env,
			     leval_args (CDR (x), env))));
	    default:
	      err_closure_code (tmp);
	    }
	  break;
	case tc_symbol:
	  x = cons (tmp, cons (cons (sym_quote, cons (x, NIL)), NIL));
	  x = leval (x, NIL);
	  goto loop;
	default:
	  p = get_user_type_hooks (TYPE (tmp));
	  if (p->leval)
	    {
	      if NULLP
		((*p->leval) (tmp, &x, &env)) return (x);
	      else
		goto loop;
	    }
	  err ("bad function", tmp);
	}
    default:
      return (x);
    }
}

LISP
lapply (LISP fcn, LISP args)
{
  struct user_type_hooks *p;
  LISP acc;
  STACK_CHECK (&fcn);
  INTERRUPT_CHECK ();
  switch TYPE
    (fcn)
    {
    case tc_subr_0:
      return (SUBR0 (fcn) ());
    case tc_subr_1:
      return (SUBR1 (fcn) (car (args)));
    case tc_subr_2:
      return (SUBR2 (fcn) (car (args), car (cdr (args))));
    case tc_subr_2n:
      acc = SUBR2 (fcn) (car (args), car (cdr (args)));
      for (args = cdr (cdr (args)); CONSP (args); args = CDR (args))
	acc = SUBR2 (fcn) (acc, CAR (args));
      return (acc);
    case tc_subr_3:
      return (SUBR3 (fcn) (car (args), car (cdr (args)), car (cdr (cdr (args)))));
    case tc_subr_4:
      return (SUBR4 (fcn) (car (args), car (cdr (args)), car (cdr (cdr (args))),
			   car (cdr (cdr (cdr (args))))));
    case tc_subr_5:
      return (SUBR5 (fcn) (car (args), car (cdr (args)), car (cdr (cdr (args))),
			   car (cdr (cdr (cdr (args)))),
			   car (cdr (cdr (cdr (cdr (args)))))));
    case tc_lsubr:
      return (SUBR1 (fcn) (args));
    case tc_fsubr:
    case tc_msubr:
    case tc_symbol:
      err ("cannot be applied", fcn);
    case tc_closure:
      switch TYPE
	(fcn->storage_as.closure.code)
	{
	case tc_cons:
	  return (leval (cdr (fcn->storage_as.closure.code),
			 extend_env (args,
				     car (fcn->storage_as.closure.code),
				     fcn->storage_as.closure.env)));
	case tc_subr_1:
	  return (SUBR1 (fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env));
	case tc_subr_2:
	  return (SUBR2 (fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env,
		   car (args)));
	case tc_subr_3:
	  return (SUBR3 (fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env,
		   car (args), car (cdr (args))));
	case tc_subr_4:
	  return (SUBR4 (fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env,
		   car (args), car (cdr (args)), car (cdr (cdr (args)))));
	case tc_subr_5:
	  return (SUBR5 (fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env,
		   car (args), car (cdr (args)), car (cdr (cdr (args))),
		   car (cdr (cdr (cdr (args))))));
	case tc_lsubr:
	  return (SUBR1 (fcn->storage_as.closure.code)
		  (cons (fcn->storage_as.closure.env, args)));
	default:
	  err_closure_code (fcn);
	}
    default:
      p = get_user_type_hooks (TYPE (fcn));
      if (p->leval)
	return err ("have eval, dont know apply", fcn);
      else
	return err ("cannot be applied", fcn);
    }
}

LISP
setvar (LISP var, LISP val, LISP env)
{
  LISP tmp;
  if NSYMBOLP
    (var) err ("wta(non-symbol) to setvar", var);
  tmp = envlookup (var, env);
  if NULLP
    (tmp) return (VCELL (var) = val);
  return (CAR (tmp) = val);
}

LISP
leval_setq (LISP args, LISP env)
{
  return (setvar (car (args), leval (car (cdr (args)), env), env));
}

LISP
syntax_define (LISP args)
{
  if SYMBOLP
    (car (args)) return (args);
  return (syntax_define (
			  cons (car (car (args)),
				cons (cons (sym_lambda,
					    cons (cdr (car (args)),
						  cdr (args))),
				      NIL))));
}

LISP
leval_define (LISP args, LISP env)
{
  LISP tmp, var, val;
  tmp = syntax_define (args);
  var = car (tmp);
  if NSYMBOLP
    (var) err ("wta(non-symbol) to define", var);
  val = leval (car (cdr (tmp)), env);
  tmp = envlookup (var, env);
  if NNULLP
    (tmp) return (CAR (tmp) = val);
  if NULLP
    (env) return (VCELL (var) = val);
  tmp = car (env);
  setcar (tmp, cons (var, car (tmp)));
  setcdr (tmp, cons (val, cdr (tmp)));
  return (val);
}

LISP
leval_if (LISP * pform, LISP * penv)
{
  LISP args, env;
  args = cdr (*pform);
  env = *penv;
  if NNULLP
    (leval (car (args), env))
      * pform = car (cdr (args));
  else
    *pform = car (cdr (cdr (args)));
  return (sym_t);
}

LISP
leval_lambda (LISP args, LISP env)
{
  LISP body;
  if NULLP
    (cdr (cdr (args)))
      body = car (cdr (args));
  else
    body = cons (sym_progn, cdr (args));
  return (closure (env, cons (arglchk (car (args)), body)));
}

LISP
leval_progn (LISP * pform, LISP * penv)
{
  LISP env, l, next;
  env = *penv;
  l = cdr (*pform);
  next = cdr (l);
  while (NNULLP (next))
    {
      leval (car (l), env);
      l = next;
      next = cdr (next);
    }
  *pform = car (l);
  return (sym_t);
}

LISP
leval_or (LISP * pform, LISP * penv)
{
  LISP env, l, next, val;
  env = *penv;
  l = cdr (*pform);
  next = cdr (l);
  while (NNULLP (next))
    {
      val = leval (car (l), env);
      if NNULLP
	(val)
	{
	  *pform = val;
	  return (NIL);
	}
      l = next;
      next = cdr (next);
    }
  *pform = car (l);
  return (sym_t);
}

LISP
leval_and (LISP * pform, LISP * penv)
{
  LISP env, l, next;
  env = *penv;
  l = cdr (*pform);
  if NULLP
    (l)
    {
      *pform = sym_t;
      return (NIL);
    }
  next = cdr (l);
  while (NNULLP (next))
    {
      if NULLP
	(leval (car (l), env))
	{
	  *pform = NIL;
	  return (NIL);
	}
      l = next;
      next = cdr (next);
    }
  *pform = car (l);
  return (sym_t);
}

LISP
leval_catch_1 (LISP forms, LISP env)
{
  LISP l, val = NIL;
  for (l = forms; NNULLP (l); l = cdr (l))
    val = leval (car (l), env);
  catch_framep = catch_framep->next;
  return (val);
}

LISP
leval_catch (LISP args, LISP env)
{
  struct catch_frame frame;
  int k;
  frame.tag = leval (car (args), env);
  frame.next = catch_framep;
  k = setjmp (frame.cframe);
  catch_framep = &frame;
  if (k == 2)
    {
      catch_framep = frame.next;
      return (frame.retval);
    }
  return (leval_catch_1 (cdr (args), env));
}

LISP
lthrow (LISP tag, LISP value)
{
  struct catch_frame *l;
  for (l = catch_framep; l; l = (*l).next)
    if (EQ ((*l).tag, tag) ||
	EQ ((*l).tag, sym_catchall))
      {
	(*l).retval = value;
	longjmp ((*l).cframe, 2);
      }
  err ("no *catch found with this tag", tag);
  return (NIL);
}

LISP
leval_let (LISP * pform, LISP * penv)
{
  LISP env, l;
  l = cdr (*pform);
  env = *penv;
  *penv = extend_env (leval_args (car (cdr (l)), env), car (l), env);
  *pform = car (cdr (cdr (l)));
  return (sym_t);
}

LISP
letstar_macro (LISP form)
{
  LISP bindings = cadr (form);
  if (NNULLP (bindings) && NNULLP (cdr (bindings)))
    setcdr (form, cons (cons (car (bindings), NIL),
			cons (cons (cintern ("let*"),
				    cons (cdr (bindings),
					  cddr (form))),
			      NIL)));
  setcar (form, cintern ("let"));
  return (form);
}

LISP
letrec_macro (LISP form)
{
  LISP letb, setb, l;
  for (letb = NIL, setb = cddr (form), l = cadr (form); NNULLP (l); l = cdr (l))
    {
      letb = cons (cons (caar (l), NIL), letb);
      setb = cons (listn (3, cintern ("set!"), caar (l), cadar (l)), setb);
    }
  setcdr (form, cons (letb, setb));
  setcar (form, cintern ("let"));
  return (form);
}

LISP
reverse (LISP l)
{
  LISP n, p;
  n = NIL;
  for (p = l; NNULLP (p); p = cdr (p))
    n = cons (car (p), n);
  return (n);
}

LISP
let_macro (LISP form)
{
  LISP p, fl, al, tmp;
  fl = NIL;
  al = NIL;
  for (p = car (cdr (form)); NNULLP (p); p = cdr (p))
    {
      tmp = car (p);
      if SYMBOLP
	(tmp)
	{
	  fl = cons (tmp, fl);
	  al = cons (NIL, al);
	}
      else
	{
	  fl = cons (car (tmp), fl);
	  al = cons (car (cdr (tmp)), al);
	}
    }
  p = cdr (cdr (form));
  if NULLP
    (cdr (p)) p = car (p);
  else
    p = cons (sym_progn, p);
  setcdr (form, cons (reverse (fl), cons (reverse (al), cons (p, NIL))));
  setcar (form, cintern ("let-internal"));
  return (form);
}

LISP
leval_quote (LISP args, LISP env)
{
  return (car (args));
}

LISP
leval_tenv (LISP args, LISP env)
{
  return (env);
}

LISP
leval_while (LISP args, LISP env)
{
  LISP l;
  while NNULLP
    (leval (car (args), env))
      for (l = cdr (args); NNULLP (l); l = cdr (l))
      leval (car (l), env);
  return (NIL);
}

LISP
symbolconc (LISP args)
{
  long size;
  LISP l, s;
  size = 0;
  tkbuffer[0] = 0;
  for (l = args; NNULLP (l); l = cdr (l))
    {
      s = car (l);
      if NSYMBOLP
	(s) err ("wta(non-symbol) to symbolconc", s);
      size = size + strlen (PNAME (s));
      if (size > TKBUFFERN)
	err ("symbolconc buffer overflow", NIL);
      strcat (tkbuffer, PNAME (s));
    }
  return (rintern (tkbuffer));
}

void
set_print_hooks (long type, void (*fcn) (LISP, struct gen_printio *))
{
  struct user_type_hooks *p;
  p = get_user_type_hooks (type);
  p->prin1 = fcn;
}

char *
subr_kind_str (long n)
{
  switch (n)
    {
    case tc_subr_0:
      return ("subr_0");
    case tc_subr_1:
      return ("subr_1");
    case tc_subr_2:
      return ("subr_2");
    case tc_subr_2n:
      return ("subr_2n");
    case tc_subr_3:
      return ("subr_3");
    case tc_subr_4:
      return ("subr_4");
    case tc_subr_5:
      return ("subr_5");
    case tc_lsubr:
      return ("lsubr");
    case tc_fsubr:
      return ("fsubr");
    case tc_msubr:
      return ("msubr");
    default:
      return ("???");
    }
}

LISP
lprin1g (LISP exp, struct gen_printio * f)
{
  LISP tmp;
  long n;
  struct user_type_hooks *p;
  STACK_CHECK (&exp);
  INTERRUPT_CHECK ();
  switch TYPE
    (exp)
    {
    case tc_nil:
      gput_st (f, "()");
      break;
    case tc_cons:
      gput_st (f, "(");
      lprin1g (car (exp), f);
      for (tmp = cdr (exp); CONSP (tmp); tmp = cdr (tmp))
	{
	  gput_st (f, " ");
	  lprin1g (car (tmp), f);
	}
      if NNULLP
	(tmp)
	{
	  gput_st (f, " . ");
	  lprin1g (tmp, f);
	}
      gput_st (f, ")");
      break;
    case tc_flonum:
      n = (long) FLONM (exp);
      if (((double) n) == FLONM (exp))
	sprintf (tkbuffer, "%ld", n);
      else
	sprintf (tkbuffer, "%g", FLONM (exp));
      gput_st (f, tkbuffer);
      break;
    case tc_symbol:
      gput_st (f, PNAME (exp));
      break;
    case tc_subr_0:
    case tc_subr_1:
    case tc_subr_2:
    case tc_subr_2n:
    case tc_subr_3:
    case tc_subr_4:
    case tc_subr_5:
    case tc_lsubr:
    case tc_fsubr:
    case tc_msubr:
      sprintf (tkbuffer, "#<%s ", subr_kind_str (TYPE (exp)));
      gput_st (f, tkbuffer);
      gput_st (f, (*exp).storage_as.subr.name);
      gput_st (f, ">");
      break;
    case tc_closure:
      gput_st (f, "#<CLOSURE ");
      if CONSP
	((*exp).storage_as.closure.code)
	{
	  lprin1g (car ((*exp).storage_as.closure.code), f);
	  gput_st (f, " ");
	  lprin1g (cdr ((*exp).storage_as.closure.code), f);
	}
      else
	lprin1g ((*exp).storage_as.closure.code, f);
      gput_st (f, ">");
      break;
    default:
      p = get_user_type_hooks (TYPE (exp));
      if (p->prin1)
	(*p->prin1) (exp, f);
      else
	{
	  sprintf (tkbuffer, "#<UNKNOWN %d %p>", TYPE (exp), exp);
	  gput_st (f, tkbuffer);
	}
    }
  return (NIL);
}

LISP
lprint (LISP exp, LISP lf)
{
  FILE *f = get_c_file (lf, siod_output);
  lprin1f (exp, f);
  if (siod_verbose_level > 0)
    fput_st (f, "\n");
  return (NIL);
}

LISP
lprin1 (LISP exp, LISP lf)
{
  FILE *f = get_c_file (lf, siod_output);
  lprin1f (exp, f);
  return (NIL);
}

LISP
lprin1f (LISP exp, FILE * f)
{
  struct gen_printio s;
  s.putc_fcn = NULL;
  s.puts_fcn = fputs_fcn;
  s.cb_argument = f;
  lprin1g (exp, &s);
  return (NIL);
}

LISP
lread (LISP f)
{
  return (lreadf (get_c_file (f, stdin)));
}

int
f_getc (FILE * f)
{
  long iflag, dflag;
  int c;
  iflag = no_interrupt (1);
  dflag = interrupt_differed;
  c = getc (f);
#ifdef VMS
  if ((dflag == 0) & interrupt_differed & (f == stdin))
    while ((c != 0) & (c != EOF))
      c = getc (f);
#endif
  no_interrupt (iflag);
  return (c);
}

void
f_ungetc (int c, FILE * f)
{
  ungetc (c, f);
}

int
flush_ws (struct gen_readio *f, char *eoferr)
{
  int c, commentp;
  commentp = 0;
  while (1)
    {
      c = GETC_FCN (f);
      if (c == EOF)
	if (eoferr)
	  err (eoferr, NIL);
	else
	  return (c);
      if (commentp)
	{
	  if (c == '\n')
	    commentp = 0;
	}
      else if (c == ';')
	commentp = 1;
      else if (!isspace (c))
	return (c);
    }
}

LISP
lreadf (FILE * f)
{
  struct gen_readio s;
  s.getc_fcn = (int (*)(void *)) f_getc;
  s.ungetc_fcn = (void (*)(int, void *)) f_ungetc;
  s.cb_argument = (char *) f;
  return (readtl (&s));
}

LISP
readtl (struct gen_readio * f)
{
  int c;
  c = flush_ws (f, (char *) NULL);
  if (c == EOF)
    return (eof_val);
  UNGETC_FCN (c, f);
  return (lreadr (f));
}

void
set_read_hooks (char *all_set, char *end_set,
		LISP (*fcn1) (int, struct gen_readio *),
		LISP (*fcn2) (char *, long, int *))
{
  user_ch_readm = all_set;
  user_te_readm = end_set;
  user_readm = fcn1;
  user_readt = fcn2;
}

LISP
lreadr (struct gen_readio *f)
{
  int c, j;
  char *p, *buffer = tkbuffer;
  STACK_CHECK (&f);
  p = buffer;
  c = flush_ws (f, "end of file inside read");
  switch (c)
    {
    case '(':
      return (lreadparen (f));
    case ')':
      err ("unexpected close paren", NIL);
    case '\'':
      return (cons (sym_quote, cons (lreadr (f), NIL)));
    case '`':
      return (cons (cintern ("+internal-backquote"), lreadr (f)));
    case ',':
      c = GETC_FCN (f);
      switch (c)
	{
	case '@':
	  p = "+internal-comma-atsign";
	  break;
	case '.':
	  p = "+internal-comma-dot";
	  break;
	default:
	  p = "+internal-comma";
	  UNGETC_FCN (c, f);
	}
      return (cons (cintern (p), lreadr (f)));
    case '"':
      return (lreadstring (f));
    case '#':
      return (lreadsharp (f));
    default:
      if ((user_readm != NULL) && strchr (user_ch_readm, c))
	return ((*user_readm) (c, f));
    }
  *p++ = c;
  for (j = 1; j < TKBUFFERN; ++j)
    {
      c = GETC_FCN (f);
      if (c == EOF)
	return (lreadtk (buffer, j));
      if (isspace (c))
	return (lreadtk (buffer, j));
      if (strchr ("()'`,;\"", c) || strchr (user_te_readm, c))
	{
	  UNGETC_FCN (c, f);
	  return (lreadtk (buffer, j));
	}
      *p++ = c;
    }
  return (err ("token larger than TKBUFFERN", NIL));
}

LISP
lreadparen (struct gen_readio * f)
{
  int c;
  LISP tmp;
  c = flush_ws (f, "end of file inside list");
  if (c == ')')
    return (NIL);
  UNGETC_FCN (c, f);
  tmp = lreadr (f);
  if EQ
    (tmp, sym_dot)
    {
      tmp = lreadr (f);
      c = flush_ws (f, "end of file inside list");
      if (c != ')')
	err ("missing close paren", NIL);
      return (tmp);
    }
  return (cons (tmp, lreadparen (f)));
}

LISP
lreadtk (char *buffer, long j)
{
  int flag;
  LISP tmp;
  int adigit;
  char *p = buffer;
  p[j] = 0;
  if (user_readt != NULL)
    {
      tmp = (*user_readt) (p, j, &flag);
      if (flag)
	return (tmp);
    }
  if (*p == '-')
    p += 1;
  adigit = 0;
  while (isdigit (*p))
    {
      p += 1;
      adigit = 1;
    }
  if (*p == '.')
    {
      p += 1;
      while (isdigit (*p))
	{
	  p += 1;
	  adigit = 1;
	}
    }
  if (!adigit)
    goto a_symbol;
  if (*p == 'e')
    {
      p += 1;
      if (*p == '-' || *p == '+')
	p += 1;
      if (!isdigit (*p))
	goto a_symbol;
      else
	p += 1;
      while (isdigit (*p))
	p += 1;
    }
  if (*p)
    goto a_symbol;
  return (flocons (atof (buffer)));
a_symbol:
  return (rintern (buffer));
}

LISP
copy_list (LISP x)
{
  if NULLP
    (x) return (NIL);
  STACK_CHECK (&x);
  return (cons (car (x), copy_list (cdr (x))));
}

LISP
apropos (LISP matchl)
{
  LISP result = NIL, l, ml;
  char *pname;
  for (l = oblistvar; CONSP (l); l = CDR (l))
    {
      pname = get_c_string (CAR (l));
      ml = matchl;
      while (CONSP (ml) && strstr (pname, get_c_string (CAR (ml))))
	ml = CDR (ml);
      if NULLP
	(ml)
	  result = cons (CAR (l), result);
    }
  return (result);
}

LISP
fopen_cg (FILE * (*fcn) (const char *, const char *), char *name, char *how)
{
  LISP sym;
  long flag;
  char errmsg[80];
  flag = no_interrupt (1);
  sym = newcell (tc_c_file);
  sym->storage_as.c_file.f = (FILE *) NULL;
  sym->storage_as.c_file.name = (char *) NULL;
  if (!(sym->storage_as.c_file.f = (*fcn) (name, how)))
    {
      SAFE_STRCPY (errmsg, "could not open ");
      SAFE_STRCAT (errmsg, name);
      err (errmsg, llast_c_errmsg (-1));
    }
  sym->storage_as.c_file.name = (char *) must_malloc (strlen (name) + 1);
  strcpy (sym->storage_as.c_file.name, name);
  no_interrupt (flag);
  return (sym);
}

LISP
fopen_c (char *name, char *how)
{
  return (fopen_cg (fopen, name, how));
}

LISP
fopen_l (LISP name, LISP how)
{
  return (fopen_c (get_c_string (name), NULLP (how) ? "r" : get_c_string (how)));
}

LISP
delq (LISP elem, LISP l)
{
  if NULLP
    (l) return (l);
  STACK_CHECK (&elem);
  if EQ
    (elem, car (l)) return (delq (elem, cdr (l)));
  setcdr (l, delq (elem, cdr (l)));
  return (l);
}

LISP
fclose_l (LISP p)
{
  long flag;
  flag = no_interrupt (1);
  if NTYPEP
    (p, tc_c_file) err ("not a file", p);
  file_gc_free (p);
  no_interrupt (flag);
  return (NIL);
}

LISP
vload (char *fname, long cflag, long rflag)
{
  LISP form, result, tail, lf, reader = NIL;
  FILE *f;
  int c, j;
  char buffer[512], *key = "parser:", *start, *end, *ftype = ".scm";
  if (rflag)
    {
      int iflag;
      iflag = no_interrupt (1);
      if ((f = fopen (fname, "r")))
	fclose (f);
      else if ((fname[0] != '/') &&
	       ((strlen (siod_lib) + strlen (fname) + 1)
		< sizeof (buffer)))
	{
	  strcpy (buffer, siod_lib);
	  strcat (buffer, "/");
	  strcat (buffer, fname);
	  if ((f = fopen (buffer, "r")))
	    {
	      fname = buffer;
	      fclose (f);
	    }
	}
      no_interrupt (iflag);
    }
  if (siod_verbose_level >= 3)
    {
      put_st ("loading ");
      put_st (fname);
      put_st ("\n");
    }
  lf = fopen_c (fname, "r");
  f = lf->storage_as.c_file.f;
  result = NIL;
  tail = NIL;
  j = 0;
  buffer[0] = 0;
  c = getc (f);
  while ((c == '#') || (c == ';'))
    {
      while (((c = getc (f)) != EOF) && (c != '\n'))
	if ((j + 1) < sizeof (buffer))
	  {
	    buffer[j] = c;
	    buffer[++j] = 0;
	  }
      if (c == '\n')
	c = getc (f);
    }
  if (c != EOF)
    ungetc (c, f);
  if ((start = strstr (buffer, key)))
    {
      for (end = &start[strlen (key)];
	   *end && isalnum (*end);
	   ++end);
      j = end - start;
      memmove (buffer, start, j);
      buffer[strlen (key) - 1] = '_';
      buffer[j] = 0;
      strcat (buffer, ftype);
      require (strcons (-1, buffer));
      buffer[j] = 0;
      reader = rintern (buffer);
      reader = funcall1 (leval (reader, NIL), reader);
      if (siod_verbose_level >= 5)
	{
	  put_st ("parser:");
	  lprin1 (reader, NIL);
	  put_st ("\n");
	}
    }
  while (1)
    {
      form = NULLP (reader) ? lread (lf) : funcall1 (reader, lf);
      if EQ
	(form, eof_val) break;
      if (siod_verbose_level >= 5)
	lprint (form, NIL);
      if (cflag)
	{
	  form = cons (form, NIL);
	  if NULLP
	    (result)
	      result = tail = form;
	  else
	    tail = setcdr (tail, form);
	}
      else
	leval (form, NIL);
    }
  fclose_l (lf);
  if (siod_verbose_level >= 3)
    put_st ("done.\n");
  return (result);
}

LISP
load (LISP fname, LISP cflag, LISP rflag)
{
  return (vload (get_c_string (fname), NULLP (cflag) ? 0 : 1, NULLP (rflag) ? 0 : 1));
}

LISP
require (LISP fname)
{
  LISP sym;
  sym = intern (string_append (cons (cintern ("*"),
				     cons (fname,
				       cons (cintern ("-loaded*"), NIL)))));
  if (NULLP (symbol_boundp (sym, NIL)) ||
      NULLP (symbol_value (sym, NIL)))
    {
      load (fname, NIL, sym_t);
      setvar (sym, sym_t, NIL);
    }
  return (sym);
}

LISP
save_forms (LISP fname, LISP forms, LISP how)
{
  char *cname, *chow = NULL;
  LISP l, lf;
  FILE *f;
  cname = get_c_string (fname);
  if EQ
    (how, NIL) chow = "w";
  else if EQ
    (how, cintern ("a")) chow = "a";
  else
    err ("bad argument to save-forms", how);
  if (siod_verbose_level >= 3)
    {
      put_st ((*chow == 'a') ? "appending" : "saving");
      put_st (" forms to ");
      put_st (cname);
      put_st ("\n");
    }
  lf = fopen_c (cname, chow);
  f = lf->storage_as.c_file.f;
  for (l = forms; NNULLP (l); l = cdr (l))
    {
      lprin1f (car (l), f);
      putc ('\n', f);
    }
  fclose_l (lf);
  if (siod_verbose_level >= 3)
    put_st ("done.\n");
  return (sym_t);
}

LISP
quit (void)
{
  return (err (NULL, NIL));
}

LISP
nullp (LISP x)
{
  if EQ
    (x, NIL) return (sym_t);
  else
    return (NIL);
}

LISP
arglchk (LISP x)
{
#if (!ENVLOOKUP_TRICK)
  LISP l;
  if SYMBOLP
    (x) return (x);
  for (l = x; CONSP (l); l = CDR (l));
  if NNULLP
    (l) err ("improper formal argument list", x);
#endif
  return (x);
}

void
file_gc_free (LISP ptr)
{
  if (ptr->storage_as.c_file.f)
    {
      fclose (ptr->storage_as.c_file.f);
      ptr->storage_as.c_file.f = (FILE *) NULL;
    }
  if (ptr->storage_as.c_file.name)
    {
      free (ptr->storage_as.c_file.name);
      ptr->storage_as.c_file.name = NULL;
    }
}

void
file_prin1 (LISP ptr, struct gen_printio *f)
{
  char *name;
  name = ptr->storage_as.c_file.name;
  gput_st (f, "#<FILE ");
  sprintf (tkbuffer, " %p", ptr->storage_as.c_file.f);
  gput_st (f, tkbuffer);
  if (name)
    {
      gput_st (f, " ");
      gput_st (f, name);
    }
  gput_st (f, ">");
}

FILE *
get_c_file (LISP p, FILE * deflt)
{
  if (NULLP (p) && deflt)
    return (deflt);
  if NTYPEP
    (p, tc_c_file) err ("not a file", p);
  if (!p->storage_as.c_file.f)
    err ("file is closed", p);
  return (p->storage_as.c_file.f);
}

LISP
lgetc (LISP p)
{
  int i;
  i = f_getc (get_c_file (p, stdin));
  return ((i == EOF) ? NIL : flocons ((double) i));
}

LISP
lungetc (LISP ii, LISP p)
{
  int i;
  if NNULLP
    (ii)
    {
      i = get_c_long (ii);
      f_ungetc (i, get_c_file (p, stdin));
    }
  return (NIL);
}

LISP
lputc (LISP c, LISP p)
{
  long flag;
  int i;
  FILE *f;
  f = get_c_file (p, siod_output);
  if FLONUMP
    (c)
      i = (int) FLONM (c);
  else
    i = *get_c_string (c);
  flag = no_interrupt (1);
  putc (i, f);
  no_interrupt (flag);
  return (NIL);
}

LISP
lputs (LISP str, LISP p)
{
  fput_st (get_c_file (p, siod_output), get_c_string (str));
  return (NIL);
}

LISP
lftell (LISP file)
{
  return (flocons ((double) ftell (get_c_file (file, NULL))));
}

LISP
lfseek (LISP file, LISP offset, LISP direction)
{
  return ((fseek (get_c_file (file, NULL), get_c_long (offset), get_c_long (direction)))
	  ? NIL : sym_t);
}

LISP
parse_number (LISP x)
{
  char *c;
  c = get_c_string (x);
  return (flocons (atof (c)));
}

void
init_subrs (void)
{
  init_subrs_1 ();
  init_subrs_a ();
}

LISP
closure_code (LISP exp)
{
  return (exp->storage_as.closure.code);
}

LISP
closure_env (LISP exp)
{
  return (exp->storage_as.closure.env);
}

LISP
lwhile (LISP form, LISP env)
{
  LISP l;
  while (NNULLP (leval (car (form), env)))
    for (l = cdr (form); NNULLP (l); l = cdr (l))
      leval (car (l), env);
  return (NIL);
}

LISP
nreverse (LISP x)
{
  LISP newp, oldp, nextp;
  newp = NIL;
  for (oldp = x; CONSP (oldp); oldp = nextp)
    {
      nextp = CDR (oldp);
      CDR (oldp) = newp;
      newp = oldp;
    }
  return (newp);
}

LISP
siod_verbose (LISP arg)
{
  if NNULLP
    (arg)
      siod_verbose_level = get_c_long (car (arg));
  return (flocons (siod_verbose_level));
}

int
siod_verbose_check (int level)
{
  return ((siod_verbose_level >= level) ? 1 : 0);
}

LISP
lruntime (void)
{
  return (cons (flocons (myruntime ()),
		cons (flocons (gc_time_taken), NIL)));
}

LISP
lrealtime (void)
{
  return (flocons (myrealtime ()));
}

LISP
caar (LISP x)
{
  return (car (car (x)));
}

LISP
cadr (LISP x)
{
  return (car (cdr (x)));
}

LISP
cdar (LISP x)
{
  return (cdr (car (x)));
}

LISP
cddr (LISP x)
{
  return (cdr (cdr (x)));
}

LISP
lrand (LISP m)
{
  long res;
  res = rand ();
  if NULLP
    (m)
      return (flocons (res));
  else
    return (flocons (res % get_c_long (m)));
}

LISP
lsrand (LISP s)
{
  srand (get_c_long (s));
  return (NIL);
}

LISP
a_true_value (void)
{
  return (sym_t);
}

LISP
poparg (LISP * ptr, LISP defaultv)
{
  LISP value;
  if NULLP
    (*ptr)
      return (defaultv);
  value = car (*ptr);
  *ptr = cdr (*ptr);
  return (value);
}

char *
last_c_errmsg (int num)
{
  int xerrno = (num < 0) ? errno : num;
  static char serrmsg[100];
  char *errmsg;
  errmsg = strerror (xerrno);
  if (!errmsg)
    {
      sprintf (serrmsg, "errno %d", xerrno);
      errmsg = serrmsg;
    }
  return (errmsg);
}

LISP
llast_c_errmsg (int num)
{
  int xerrno = (num < 0) ? errno : num;
  char *errmsg = strerror (xerrno);
  if (!errmsg)
    return (flocons (xerrno));
  return (cintern (errmsg));
}

LISP
lllast_c_errmsg (void)
{
  return (llast_c_errmsg (-1));
}

LISP
help (void)
{
  fprintf (siod_output, "HELP for SIOD, Version %s\n", siod_version ());
  fprintf (siod_output, "For the latest Script-Fu tips, tutorials, & info:\n");
  fprintf (siod_output, "\thttp://www.xcf.berkeley.edu/~gimp/script-fu/script-fu.html\n\n");

  return NIL;
}

size_t
safe_strlen (const char *s, size_t size)
{
  char *end;
  if ((end = (char *) memchr (s, 0, size)))
    return (end - s);
  else
    return (size);
}

char *
safe_strcpy (char *s1, size_t size1, const char *s2)
{
  size_t len2;
  if (size1 == 0)
    return (s1);
  len2 = strlen (s2);
  if (len2 < size1)
    {
      if (len2)
	memcpy (s1, s2, len2);
      s1[len2] = 0;
    }
  else
    {
      memcpy (s1, s2, size1);
      s1[size1 - 1] = 0;
    }
  return (s1);
}

char *
safe_strcat (char *s1, size_t size1, const char *s2)
{
  size_t len1;
  len1 = safe_strlen (s1, size1);
  safe_strcpy (&s1[len1], size1 - len1, s2);
  return (s1);
}

static LISP
parser_read (LISP ignore)
{
  return (leval (cintern ("read"), NIL));
}

void
init_subrs_1 (void)
{
  init_subr_2 ("cons", cons);
  init_subr_1 ("car", car);
  init_subr_1 ("cdr", cdr);
  init_subr_2 ("set-car!", setcar);
  init_subr_2 ("set-cdr!", setcdr);
  init_subr_2n ("+", plus);
  init_subr_2n ("-", difference);
  init_subr_2n ("*", ltimes);
  init_subr_2n ("/", Quotient);
  init_subr_2n ("min", lmin);
  init_subr_2n ("max", lmax);
  init_subr_1 ("abs", lllabs);
  init_subr_1 ("sqrt", lsqrt);
  init_subr_2 (">", greaterp);
  init_subr_2 ("<", lessp);
  init_subr_2 (">=", greaterEp);
  init_subr_2 ("<=", lessEp);
  init_subr_2 ("eq?", eq);
  init_subr_2 ("eqv?", eql);
  init_subr_2 ("=", eql);
  init_subr_2 ("assq", assq);
  init_subr_2 ("delq", delq);
  init_subr_1 ("read", lread);
  init_subr_1 ("parser_read", parser_read);
  setvar (cintern ("*parser_read.scm-loaded*"), sym_t, NIL);
  init_subr_0 ("eof-val", get_eof_val);
  init_subr_2 ("print", lprint);
  init_subr_2 ("prin1", lprin1);
  init_subr_2 ("eval", leval);
  init_subr_2 ("apply", lapply);
  init_fsubr ("define", leval_define);
  init_fsubr ("lambda", leval_lambda);
  init_msubr ("if", leval_if);
  init_fsubr ("while", leval_while);
  init_msubr ("begin", leval_progn);
  init_fsubr ("set!", leval_setq);
  init_msubr ("or", leval_or);
  init_msubr ("and", leval_and);
  init_fsubr ("*catch", leval_catch);
  init_subr_2 ("*throw", lthrow);
  init_fsubr ("quote", leval_quote);
  init_lsubr ("apropos", apropos);
  init_lsubr ("verbose", siod_verbose);
  init_subr_1 ("copy-list", copy_list);
  init_lsubr ("gc-status", gc_status);
  init_lsubr ("gc", user_gc);
  init_subr_3 ("load", load);
  init_subr_1 ("require", require);
  init_subr_1 ("pair?", consp);
  init_subr_1 ("symbol?", symbolp);
  init_subr_1 ("number?", numberp);
  init_msubr ("let-internal", leval_let);
  init_subr_1 ("let-internal-macro", let_macro);
  init_subr_1 ("let*-macro", letstar_macro);
  init_subr_1 ("letrec-macro", letrec_macro);
  init_subr_2 ("symbol-bound?", symbol_boundp);
  init_subr_2 ("symbol-value", symbol_value);
  init_subr_3 ("set-symbol-value!", setvar);
  init_fsubr ("the-environment", leval_tenv);
  init_subr_2 ("error", lerr);
  init_subr_0 ("quit", quit);
  init_subr_1 ("not", nullp);
  init_subr_1 ("null?", nullp);
  init_subr_2 ("env-lookup", envlookup);
  init_subr_1 ("reverse", reverse);
  init_lsubr ("symbolconc", symbolconc);
  init_subr_3 ("save-forms", save_forms);
  init_subr_2 ("fopen", fopen_l);
  init_subr_1 ("fclose", fclose_l);
  init_subr_1 ("getc", lgetc);
  init_subr_2 ("ungetc", lungetc);
  init_subr_2 ("putc", lputc);
  init_subr_2 ("puts", lputs);
  init_subr_1 ("ftell", lftell);
  init_subr_3 ("fseek", lfseek);
  init_subr_1 ("parse-number", parse_number);
  init_subr_2 ("%%stack-limit", stack_limit);
  init_subr_1 ("intern", intern);
  init_subr_2 ("%%closure", closure);
  init_subr_1 ("%%closure-code", closure_code);
  init_subr_1 ("%%closure-env", closure_env);
  init_fsubr ("while", lwhile);
  init_subr_1 ("nreverse", nreverse);
  init_subr_0 ("allocate-heap", allocate_aheap);
  init_subr_1 ("gc-info", gc_info);
  init_subr_0 ("runtime", lruntime);
  init_subr_0 ("realtime", lrealtime);
  init_subr_1 ("caar", caar);
  init_subr_1 ("cadr", cadr);
  init_subr_1 ("cdar", cdar);
  init_subr_1 ("cddr", cddr);
  init_subr_1 ("rand", lrand);
  init_subr_1 ("srand", lsrand);
  init_subr_0 ("last-c-error", lllast_c_errmsg);
  init_subr_0 ("help", help);
  init_slib_version ();
}


/* err0,pr,prp are convenient to call from the C-language debugger */

void
err0 (void)
{
  err ("0", NIL);
}

void
pr (LISP p)
{
  if (looks_pointerp (p))
    lprint (p, NIL);
  else
    put_st ("invalid\n");
}

void
prp (LISP * p)
{
  if (!p)
    return;
  pr (*p);
}
