
/*    COPYRIGHT (c) 1992-1994 BY
 *    MITECH CORPORATION, ACTON, MASSACHUSETTS.
 *    See the source file SLIB.C for more information.

 (trace procedure1 procedure2 ...)
 (untrace procedure1 procedure2 ...)

 Currently only user-defined procedures can be traced.
 Fancy printing features such as indentation based on
 recursion level will also have to wait for a future version.


 */

#include <stdio.h>
#include <setjmp.h>
#include "siod.h"
#include "siodp.h"

static void 
init_trace_version (void)
{
  setvar (cintern ("*trace-version*"),
	  cintern ("$Id$"),
	  NIL);
}


static long tc_closure_traced = 0;

static LISP sym_traced = NIL;
static LISP sym_quote = NIL;
static LISP sym_begin = NIL;

LISP ltrace_fcn_name (LISP body);
LISP ltrace_1 (LISP fcn_name, LISP env);
LISP ltrace (LISP fcn_names, LISP env);
LISP luntrace_1 (LISP fcn);
LISP luntrace (LISP fcns);
static void ct_gc_scan (LISP ptr);
static LISP ct_gc_mark (LISP ptr);
void ct_prin1 (LISP ptr, struct gen_printio *f);
LISP ct_eval (LISP ct, LISP * px, LISP * penv);

LISP 
ltrace_fcn_name (LISP body)
{
  LISP tmp;
  if NCONSP
    (body) return (NIL);
  if NEQ
    (CAR (body), sym_begin) return (NIL);
  tmp = CDR (body);
  if NCONSP
    (tmp) return (NIL);
  tmp = CAR (tmp);
  if NCONSP
    (tmp) return (NIL);
  if NEQ
    (CAR (tmp), sym_quote) return (NIL);
  tmp = CDR (tmp);
  if NCONSP
    (tmp) return (NIL);
  return (CAR (tmp));
}

LISP 
ltrace_1 (LISP fcn_name, LISP env)
{
  LISP fcn, code;
  fcn = leval (fcn_name, env);
  if (TYPE (fcn) == tc_closure)
    {
      code = fcn->storage_as.closure.code;
      if NULLP
	(ltrace_fcn_name (cdr (code)))
	  setcdr (code, cons (sym_begin,
			      cons (cons (sym_quote, cons (fcn_name, NIL)),
				    cons (cdr (code), NIL))));
      fcn->type = tc_closure_traced;
    }
  else if (TYPE (fcn) == tc_closure_traced)
    ;
  else
    err ("not a closure, cannot trace", fcn);
  return (NIL);
}

LISP 
ltrace (LISP fcn_names, LISP env)
{
  LISP l;
  for (l = fcn_names; NNULLP (l); l = cdr (l))
    ltrace_1 (car (l), env);
  return (NIL);
}

LISP 
luntrace_1 (LISP fcn)
{
  if (TYPE (fcn) == tc_closure)
    ;
  else if (TYPE (fcn) == tc_closure_traced)
    fcn->type = tc_closure;
  else
    err ("not a closure, cannot untrace", fcn);
  return (NIL);
}

LISP 
luntrace (LISP fcns)
{
  LISP l;
  for (l = fcns; NNULLP (l); l = cdr (l))
    luntrace_1 (car (l));
  return (NIL);
}

static void 
ct_gc_scan (LISP ptr)
{
  CAR (ptr) = gc_relocate (CAR (ptr));
  CDR (ptr) = gc_relocate (CDR (ptr));
}

static LISP 
ct_gc_mark (LISP ptr)
{
  gc_mark (ptr->storage_as.closure.code);
  return (ptr->storage_as.closure.env);
}

void 
ct_prin1 (LISP ptr, struct gen_printio *f)
{
  gput_st (f, "#<CLOSURE(TRACED) ");
  lprin1g (car (ptr->storage_as.closure.code), f);
  gput_st (f, " ");
  lprin1g (cdr (ptr->storage_as.closure.code), f);
  gput_st (f, ">");
}

LISP 
ct_eval (LISP ct, LISP * px, LISP * penv)
{
  LISP fcn_name, args, env, result, l;
  fcn_name = ltrace_fcn_name (cdr (ct->storage_as.closure.code));
  args = leval_args (CDR (*px), *penv);
  fput_st (stdout, "->");
  lprin1f (fcn_name, stdout);
  for (l = args; NNULLP (l); l = cdr (l))
    {
      fput_st (stdout, " ");
      lprin1f (car (l), stdout);
    }
  fput_st (stdout, "\n");
  env = extend_env (args,
		    car (ct->storage_as.closure.code),
		    ct->storage_as.closure.env);
  result = leval (cdr (ct->storage_as.closure.code), env);
  fput_st (stdout, "<-");
  lprin1f (fcn_name, stdout);
  fput_st (stdout, " ");
  lprin1f (result, stdout);
  fput_st (stdout, "\n");
  *px = result;
  return (NIL);
}

void 
init_trace (void)
{
  long j;
  tc_closure_traced = allocate_user_tc ();
  set_gc_hooks (tc_closure_traced,
		NULL,
		ct_gc_mark,
		ct_gc_scan,
		NULL,
		&j);
  gc_protect_sym (&sym_traced, "*traced*");
  setvar (sym_traced, NIL, NIL);
  gc_protect_sym (&sym_begin, "begin");
  gc_protect_sym (&sym_quote, "quote");
  set_print_hooks (tc_closure_traced, ct_prin1);
  set_eval_hooks (tc_closure_traced, ct_eval);
  init_fsubr ("trace", ltrace);
  init_lsubr ("untrace", luntrace);
  init_trace_version ();
}
