/* re.c */
/* Henry Spencer's implementation of Regular Expressions,
   used for TinyScheme */
/* Refurbished by Stephen Gildea */
/* Ported to GRegex by Michael Natterer */

#include "config.h"

#include "tinyscheme/scheme-private.h"
#include "script-fu-regex.h"


static pointer foreign_re_match (scheme  *sc,
                                 pointer  args);
static void    set_vector_elem  (pointer  vec,
                                 int      ielem,
                                 pointer  newel);


#if 0
static char* utilities=";; return the substring of STRING matched in MATCH-VECTOR, \n"
";; the Nth subexpression match (default 0).\n"
"(define (re-match-nth string match-vector . n)\n"
"  (let ((n (if (pair? n) (car n) 0)))\n"
"    (substring string (car (vector-ref match-vector n))\n"
"                    (cdr (vector-ref match-vector n)))))\n"
"(define (re-before-nth string match-vector . n)\n"
"  (let ((n (if (pair? n) (car n) 0)))\n"
"    (substring string 0 (car (vector-ref match-vector n)))))\n"
"(define (re-after-nth string match-vector . n)\n"
"  (let ((n (if (pair? n) (car n) 0)))\n"
"    (substring string (cdr (vector-ref match-vector n))\n"
"             (string-length string))))\n";
#endif

void
init_re (scheme *sc)
{
  sc->vptr->scheme_define (sc,
                           sc->global_env,
                           sc->vptr->mk_symbol(sc,"re-match"),
                           sc->vptr->mk_foreign_func(sc, foreign_re_match));

#if 0
  sc->vptr->load_string (sc,utilities);
#endif
}

static pointer
foreign_re_match (scheme  *sc,
                  pointer  args)
{
  pointer   retval = sc->F;
  gboolean  success;
  GRegex   *regex;
  pointer   first_arg, second_arg;
  pointer   third_arg = sc->NIL;
  char     *string;
  char     *pattern;
  int       num = 0;

  if (!((args != sc->NIL)
        && sc->vptr->is_string ((first_arg = sc->vptr->pair_car (args)))
        && (args = sc->vptr->pair_cdr (args))
        && sc->vptr->is_pair (args)
        && sc->vptr->is_string ((second_arg = sc->vptr->pair_car (args)))))
    {
      return sc->F;
    }

  pattern = sc->vptr->string_value (first_arg);
  string  = sc->vptr->string_value (second_arg);

  args = sc->vptr->pair_cdr (args);

  if (args != sc->NIL)
    {
      if (!(sc->vptr->is_pair (args)
            && sc->vptr->is_vector ((third_arg = sc->vptr->pair_car (args)))))
        {
          return sc->F;
        }
      else
        {
          num = third_arg->_object._number.value.ivalue;
        }
    }

  regex = g_regex_new (pattern, G_REGEX_EXTENDED, 0, NULL);
  if (! regex)
    return sc->F;

  if (! num)
    {
      success = g_regex_match (regex, string, 0, NULL);
    }
  else
    {
      GMatchInfo *match_info;
      gint        i;

      success = g_regex_match (regex, string, 0, &match_info);

      for (i = 0; i < num; i++)
        {
          gint start, end;

          g_match_info_fetch_pos (match_info, i, &start, &end);

#undef cons
          set_vector_elem (third_arg, i,
                           sc->vptr->cons(sc,
                                          sc->vptr->mk_integer(sc, start),
                                          sc->vptr->mk_integer(sc, end)));
        }

      g_match_info_free (match_info);
    }

  if (success)
    retval = sc->T;

  g_regex_unref (regex);

  return retval;
}

static void
set_vector_elem (pointer vec,
                 int     ielem,
                 pointer newel)
{
  int n = ielem / 2;

  if (ielem % 2 == 0)
    {
      vec[1 + n]._object._cons._car = newel;
    }
  else
    {
      vec[1 + n]._object._cons._cdr = newel;
    }
}
