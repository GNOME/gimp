/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Based on re.c
 *
 * Henry Spencer's implementation of Regular Expressions,
 * used for TinyScheme
 *
 * Refurbished by Stephen Gildea
 *
 * Ported to GRegex and de-uglified by Michael Natterer
 */

#include "config.h"

#include "tinyscheme/scheme-private.h"
#include "script-fu-regex.h"


/*  local function prototypes  */

static pointer foreign_re_match (scheme  *sc,
                                 pointer  args);
static void    set_vector_elem  (pointer  vec,
                                 int      ielem,
                                 pointer  newel);


/*  public functions  */

void
script_fu_regex_init (scheme *sc)
{
  sc->vptr->scheme_define (sc,
                           sc->global_env,
                           sc->vptr->mk_symbol(sc,"re-match"),
                           sc->vptr->mk_foreign_func(sc, foreign_re_match));

#if 0
  sc->vptr->load_string
    (sc,
     ";; return the substring of STRING matched in MATCH-VECTOR,\n"
     ";; the Nth subexpression match (default 0).\n"
     "\n"
     "(define (re-match-nth string match-vector . n)\n"
     "  (let ((n (if (pair? n) (car n) 0)))\n"
     "    (substring string (car (vector-ref match-vector n))\n"
     "                      (cdr (vector-ref match-vector n)))))\n"
     "(define (re-before-nth string match-vector . n)\n"
     "  (let ((n (if (pair? n) (car n) 0)))\n"
     "    (substring string 0 (car (vector-ref match-vector n)))))\n"
     "(define (re-after-nth string match-vector . n)\n"
     "  (let ((n (if (pair? n) (car n) 0)))\n"
     "    (substring string (cdr (vector-ref match-vector n))\n"
     "                      (string-length string))))\n");
#endif
}


/*  private functions  */

static pointer
foreign_re_match (scheme  *sc,
                  pointer  args)
{
  pointer   retval = sc->F;
  gboolean  success;
  gboolean  is_valid_utf8;
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

  is_valid_utf8 = g_utf8_validate (string, -1, NULL);

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

          if (is_valid_utf8)
            {
              start = g_utf8_pointer_to_offset (string, string + start);
              end   = g_utf8_pointer_to_offset (string, string + end);
            }

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
