#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "siod.h"

/* OSF/1 doc says that POSIX and XPG4 include regcomp in libc.
   So we might as well set ourselves up to take advantage of it.
   This functionality is also available in hpux, and is also provided
   by the FSF's librx package, so if you can use that if your
   operating system vendor doesn't supply it.
 */

static void
init_regex_version (void)
{
  setvar (cintern ("*regex-version*"),
	  cintern ("$Id$"),
	  NIL);
}

long tc_regex = 0;

struct tc_regex
  {
    int compflag;
    size_t nmatch;
    regex_t *r;
    regmatch_t *m;
  };

struct tc_regex *
get_tc_regex (LISP ptr)
{
  if NTYPEP
    (ptr, tc_regex) err ("not a regular expression", ptr);
  return ((struct tc_regex *) ptr->storage_as.string.data);
}

LISP
regcomp_l (LISP pattern, LISP flags)
{
  long iflag, iflags;
  char *str, errbuff[1024];
  int error;
  LISP result;
  struct tc_regex *h;
  iflags = NNULLP (flags) ? get_c_long (flags) : 0;
  str = get_c_string (pattern);
  iflag = no_interrupt (1);
  result = cons (NIL, NIL);
  h = (struct tc_regex *) must_malloc (sizeof (struct tc_regex));
  h->compflag = 0;
  h->nmatch = 0;
  h->r = NULL;
  h->m = NULL;
  result->type = tc_regex;
  result->storage_as.string.data = (char *) h;
  h->r = (regex_t *) must_malloc (sizeof (regex_t));
  if ((error = regcomp (h->r, str, iflags)))
    {
      regerror (error, h->r, errbuff, sizeof (errbuff));
      return (err (errbuff, pattern));
    }
  h->compflag = 1;
  if (iflags & REG_NOSUB)
    {
      no_interrupt (iflag);
      return (result);
    }
  h->nmatch = h->r->re_nsub + 1;
  h->m = (regmatch_t *) must_malloc (sizeof (regmatch_t) * h->nmatch);
  no_interrupt (iflag);
  return (result);
}

LISP
regerror_l (LISP code, LISP ptr)
{
  char errbuff[1024];
  regerror (get_c_long (code), get_tc_regex (ptr)->r, errbuff, sizeof (errbuff));
  return (strcons (strlen (errbuff), errbuff));
}

LISP
regexec_l (LISP ptr, LISP str, LISP eflags)
{
  size_t j;
  int error;
  LISP result;
  struct tc_regex *h;
  h = get_tc_regex (ptr);
  if ((error = regexec (h->r,
		       get_c_string (str),
		       h->nmatch,
		       h->m,
		       NNULLP (eflags) ? get_c_long (eflags) : 0)))
    return (flocons (error));
  for (j = 0, result = NIL; j < h->nmatch; ++j)
    result = cons (cons (flocons (h->m[j].rm_so),
			 flocons (h->m[j].rm_eo)),
		   result);
  return (nreverse (result));
}

void
regex_gc_free (LISP ptr)
{
  struct tc_regex *h;
  if ((h = (struct tc_regex *) ptr->storage_as.string.data))
    {
      if ((h->compflag) && h->r)
	regfree (h->r);
      if (h->r)
	{
	  free (h->r);
	  h->r = NULL;
	}
      if (h->m)
	{
	  free (h->m);
	  h->m = NULL;
	}
      free (h);
      ptr->storage_as.string.data = NULL;
    }
}

void
regex_prin1 (LISP ptr, struct gen_printio *f)
{
  char buffer[256];
  regex_t *p;
  p = get_tc_regex (ptr)->r;
  sprintf (buffer, "#<REGEX %p nsub=%d",
	   p, p->re_nsub);
  gput_st (f, buffer);
#if defined(__osf__)
  sprintf (buffer, ", len=%d flags=%X",
	   p->re_len, p->re_cflags);
  gput_st (f, buffer);
#endif
  gput_st (f, ">");
}

void
init_regex (void)
{
  long j;
  tc_regex = allocate_user_tc ();
  set_gc_hooks (tc_regex,
		NULL,
		NULL,
		NULL,
		regex_gc_free,
		&j);
  set_print_hooks (tc_regex, regex_prin1);
  init_subr_2 ("regcomp", regcomp_l);
  init_subr_2 ("regerror", regerror_l);
  init_subr_3 ("regexec", regexec_l);
  setvar (cintern ("REG_EXTENDED"), flocons (REG_EXTENDED), NIL);
  setvar (cintern ("REG_ICASE"), flocons (REG_ICASE), NIL);
  setvar (cintern ("REG_NOSUB"), flocons (REG_NOSUB), NIL);
  setvar (cintern ("REG_NEWLINE"), flocons (REG_NEWLINE), NIL);

  setvar (cintern ("REG_NOTBOL"), flocons (REG_NOTBOL), NIL);
  setvar (cintern ("REG_NOTEOL"), flocons (REG_NOTEOL), NIL);

  setvar (cintern ("REG_NOMATCH"), flocons (REG_NOMATCH), NIL);
  setvar (cintern ("REG_BADPAT"), flocons (REG_BADPAT), NIL);
  setvar (cintern ("REG_ECOLLATE"), flocons (REG_ECOLLATE), NIL);
  setvar (cintern ("REG_ECTYPE"), flocons (REG_ECTYPE), NIL);
  setvar (cintern ("REG_EESCAPE"), flocons (REG_EESCAPE), NIL);
  setvar (cintern ("REG_ESUBREG"), flocons (REG_ESUBREG), NIL);
  setvar (cintern ("REG_EBRACK"), flocons (REG_EBRACK), NIL);
  setvar (cintern ("REG_EPAREN"), flocons (REG_EPAREN), NIL);
  setvar (cintern ("REG_EBRACE"), flocons (REG_EBRACE), NIL);
  setvar (cintern ("REG_BADBR"), flocons (REG_BADBR), NIL);
  setvar (cintern ("REG_ERANGE"), flocons (REG_ERANGE), NIL);
  setvar (cintern ("REG_ESPACE"), flocons (REG_ESPACE), NIL);
  setvar (cintern ("REG_BADRPT"), flocons (REG_BADRPT), NIL);
#ifdef REG_ECHAR
  setvar (cintern ("REG_ECHAR"), flocons (REG_ECHAR), NIL);
#endif
  init_regex_version ();
}
