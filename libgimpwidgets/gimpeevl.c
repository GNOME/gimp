/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaeevl.c
 * Copyright (C) 2008 Fredrik Alstromer <roe@excu.se>
 * Copyright (C) 2008 Martin Nordholts <martinn@svn.gnome.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* Introducing eevl eva, the evaluator. A straightforward recursive
 * descent parser, no fuss, no new dependencies. The lexer is hand
 * coded, tedious, not extremely fast but works. It evaluates the
 * expression as it goes along, and does not create a parse tree or
 * anything, and will not optimize anything. It uses doubles for
 * precision, with the given use case, that's enough to combat any
 * rounding errors (as opposed to optimizing the evaluation).
 *
 * It relies on external unit resolving through a callback and does
 * elementary dimensionality constraint check (e.g. "2 mm + 3 px * 4
 * in" is an error, as L + L^2 is a mismatch). It uses setjmp/longjmp
 * for try/catch like pattern on error, it uses g_strtod() for numeric
 * conversions and it's non-destructive in terms of the parameters, and
 * it's reentrant.
 *
 * EBNF:
 *
 *   expression    ::= term { ('+' | '-') term }*  |
 *                     <empty string> ;
 *
 *   term          ::= ratio { ( '*' | '/' ) ratio }* ;
 *
 *   ratio         ::= signed factor { ':' signed factor }* ;
 *
 *   signed factor ::= ( '+' | '-' )? factor ;
 *
 *   factor        ::= quantity ( '^' signed factor )? ;
 *
 *   quantity      ::= number unit? | '(' expression ')' ;
 *
 *   number        ::= ? what g_strtod() consumes ? ;
 *
 *   unit          ::= simple unit ( '^' signed factor )? ;
 *
 *   simple unit   ::= ? what not g_strtod() consumes and not whitespace ? ;
 *
 * The code should match the EBNF rather closely (except for the
 * non-terminal unit factor, which is inlined into factor) for
 * maintainability reasons.
 *
 * It will allow 1++1 and 1+-1 (resulting in 2 and 0, respectively),
 * but I figured one might want that, and I don't think it's going to
 * throw anyone off.
 */

#include "config.h"

#include <setjmp.h>
#include <string.h>

#include <glib-object.h>

#include "libligmamath/ligmamath.h"

#include "ligmaeevl.h"
#include "ligmawidgets-error.h"

#include "libligma/libligma-intl.h"


typedef enum
{
  LIGMA_EEVL_TOKEN_NUM        = 30000,
  LIGMA_EEVL_TOKEN_IDENTIFIER = 30001,

  LIGMA_EEVL_TOKEN_ANY        = 40000,

  LIGMA_EEVL_TOKEN_END        = 50000
} LigmaEevlTokenType;


typedef struct
{
  LigmaEevlTokenType type;

  union
  {
    gdouble fl;

    struct
    {
      const gchar *c;
      gint         size;
    };

  } value;

} LigmaEevlToken;

typedef struct
{
  const gchar     *string;
  LigmaEevlOptions  options;

  LigmaEevlToken    current_token;
  const gchar     *start_of_current_token;

  jmp_buf          catcher;
  const gchar     *error_message;

} LigmaEevl;


static void             ligma_eevl_init                     (LigmaEevl              *eva,
                                                            const gchar           *string,
                                                            const LigmaEevlOptions *options);
static LigmaEevlQuantity ligma_eevl_complete                 (LigmaEevl              *eva);
static LigmaEevlQuantity ligma_eevl_expression               (LigmaEevl              *eva);
static LigmaEevlQuantity ligma_eevl_term                     (LigmaEevl              *eva);
static LigmaEevlQuantity ligma_eevl_ratio                    (LigmaEevl              *eva);
static LigmaEevlQuantity ligma_eevl_signed_factor            (LigmaEevl              *eva);
static LigmaEevlQuantity ligma_eevl_factor                   (LigmaEevl              *eva);
static LigmaEevlQuantity ligma_eevl_quantity                 (LigmaEevl              *eva);
static gboolean         ligma_eevl_accept                   (LigmaEevl              *eva,
                                                            LigmaEevlTokenType      token_type,
                                                            LigmaEevlToken         *consumed_token);
static void             ligma_eevl_lex                      (LigmaEevl              *eva);
static void             ligma_eevl_lex_accept_count          (LigmaEevl              *eva,
                                                            gint                   count,
                                                            LigmaEevlTokenType      token_type);
static void             ligma_eevl_lex_accept_to             (LigmaEevl              *eva,
                                                            gchar                 *to,
                                                            LigmaEevlTokenType      token_type);
static void             ligma_eevl_move_past_whitespace     (LigmaEevl              *eva);
static gboolean         ligma_eevl_unit_identifier_start    (gunichar               c);
static gboolean         ligma_eevl_unit_identifier_continue (gunichar               c);
static gint             ligma_eevl_unit_identifier_size     (const gchar           *s,
                                                            gint                   start);
static void             ligma_eevl_expect                   (LigmaEevl              *eva,
                                                            LigmaEevlTokenType      token_type,
                                                            LigmaEevlToken         *value);
static void             ligma_eevl_error                    (LigmaEevl              *eva,
                                                            gchar                 *msg);


/**
 * ligma_eevl_evaluate:
 * @string:        The NULL-terminated string to be evaluated.
 * @options:       Evaluations options.
 * @result:        Result of evaluation.
 * @error_pos:     Will point to the position within the string,
 *                 before which the parse / evaluation error
 *                 occurred. Will be set to null if no error occurred.
 * @error_message: Will point to a static string with a semi-descriptive
 *                 error message if parsing / evaluation failed.
 *
 * Evaluates the given arithmetic expression, along with an optional dimension
 * analysis, and basic unit conversions.
 *
 * All units conversions factors are relative to some implicit
 * base-unit (which in LIGMA is inches). This is also the unit of the
 * returned value.
 *
 * Returns: A #LigmaEevlQuantity with a value given in the base unit along with
 * the order of the dimension (i.e. if the base unit is inches, a dimension
 * order of two means in^2).
 **/
gboolean
ligma_eevl_evaluate (const gchar            *string,
                    const LigmaEevlOptions  *options,
                    LigmaEevlQuantity       *result,
                    const gchar           **error_pos,
                    GError                **error)
{
  LigmaEevl eva;

  g_return_val_if_fail (g_utf8_validate (string, -1, NULL), FALSE);
  g_return_val_if_fail (options != NULL, FALSE);
  g_return_val_if_fail (options->unit_resolver_proc != NULL, FALSE);
  g_return_val_if_fail (result != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ligma_eevl_init (&eva,
                  string,
                  options);

  if (!setjmp (eva.catcher))  /* try... */
    {
      *result = ligma_eevl_complete (&eva);

      return TRUE;
    }
  else   /* catch.. */
    {
      if (error_pos)
        *error_pos = eva.start_of_current_token;

      g_set_error_literal (error,
                           LIGMA_WIDGETS_ERROR,
                           LIGMA_WIDGETS_PARSE_ERROR,
                           eva.error_message);

      return FALSE;
    }
}

static void
ligma_eevl_init (LigmaEevl              *eva,
                const gchar           *string,
                const LigmaEevlOptions *options)
{
  eva->string  = string;
  eva->options = *options;

  eva->current_token.type  = LIGMA_EEVL_TOKEN_END;

  eva->error_message       = NULL;

  /* Preload symbol... */
  ligma_eevl_lex (eva);
}

static LigmaEevlQuantity
ligma_eevl_complete (LigmaEevl *eva)
{
  LigmaEevlQuantity result = {0, 0};
  LigmaEevlQuantity default_unit_factor;
  gdouble          default_unit_offset;

  /* Empty expression evaluates to 0 */
  if (ligma_eevl_accept (eva, LIGMA_EEVL_TOKEN_END, NULL))
    return result;

  result = ligma_eevl_expression (eva);

  /* There should be nothing left to parse by now */
  ligma_eevl_expect (eva, LIGMA_EEVL_TOKEN_END, 0);

  eva->options.unit_resolver_proc (NULL,
                                   &default_unit_factor,
                                   &default_unit_offset,
                                   eva->options.data);

  /* Entire expression is dimensionless, apply default unit if
   * applicable
   */
  if (result.dimension == 0 && default_unit_factor.dimension != 0)
    {
      result.value     /= default_unit_factor.value;
      result.value     += default_unit_offset;
      result.dimension  = default_unit_factor.dimension;
    }
  return result;
}

static LigmaEevlQuantity
ligma_eevl_expression (LigmaEevl *eva)
{
  gboolean         subtract;
  LigmaEevlQuantity evaluated_terms;

  evaluated_terms = ligma_eevl_term (eva);

  /* continue evaluating terms, chained with + or -. */
  for (subtract = FALSE;
       ligma_eevl_accept (eva, '+', NULL) ||
       (subtract = ligma_eevl_accept (eva, '-', NULL));
       subtract = FALSE)
    {
      LigmaEevlQuantity new_term = ligma_eevl_term (eva);

      /* If dimensions mismatch, attempt default unit assignment */
      if (new_term.dimension != evaluated_terms.dimension)
        {
          LigmaEevlQuantity default_unit_factor;
          gdouble          default_unit_offset;

          eva->options.unit_resolver_proc (NULL,
                                           &default_unit_factor,
                                           &default_unit_offset,
                                           eva->options.data);

          if (new_term.dimension == 0 &&
              evaluated_terms.dimension == default_unit_factor.dimension)
            {
              new_term.value     /= default_unit_factor.value;
              new_term.value     += default_unit_offset;
              new_term.dimension  = default_unit_factor.dimension;
            }
          else if (evaluated_terms.dimension == 0 &&
                   new_term.dimension == default_unit_factor.dimension)
            {
              evaluated_terms.value     /= default_unit_factor.value;
              evaluated_terms.value     += default_unit_offset;
              evaluated_terms.dimension  = default_unit_factor.dimension;
            }
          else
            {
              ligma_eevl_error (eva, "Dimension mismatch during addition");
            }
        }

      evaluated_terms.value += (subtract ? -new_term.value : new_term.value);
    }

  return evaluated_terms;
}

static LigmaEevlQuantity
ligma_eevl_term (LigmaEevl *eva)
{
  gboolean         division;
  LigmaEevlQuantity evaluated_ratios;

  evaluated_ratios = ligma_eevl_ratio (eva);

  for (division = FALSE;
       ligma_eevl_accept (eva, '*', NULL) ||
       (division = ligma_eevl_accept (eva, '/', NULL));
       division = FALSE)
    {
      LigmaEevlQuantity new_ratio = ligma_eevl_ratio (eva);

      if (division)
        {
          evaluated_ratios.value     /= new_ratio.value;
          evaluated_ratios.dimension -= new_ratio.dimension;
        }
      else
        {
          evaluated_ratios.value     *= new_ratio.value;
          evaluated_ratios.dimension += new_ratio.dimension;
        }
    }

  return evaluated_ratios;
}

static LigmaEevlQuantity
ligma_eevl_ratio (LigmaEevl *eva)
{
  LigmaEevlQuantity evaluated_signed_factors;

  if (! eva->options.ratio_expressions)
    return ligma_eevl_signed_factor (eva);

  evaluated_signed_factors = ligma_eevl_signed_factor (eva);

  while (ligma_eevl_accept (eva, ':', NULL))
    {
      LigmaEevlQuantity new_signed_factor = ligma_eevl_signed_factor (eva);

      if (eva->options.ratio_invert)
        {
          LigmaEevlQuantity temp;

          temp                     = evaluated_signed_factors;
          evaluated_signed_factors = new_signed_factor;
          new_signed_factor        = temp;
        }

      evaluated_signed_factors.value     *= eva->options.ratio_quantity.value /
                                            new_signed_factor.value;
      evaluated_signed_factors.dimension += eva->options.ratio_quantity.dimension -
                                            new_signed_factor.dimension;
    }

  return evaluated_signed_factors;
}

static LigmaEevlQuantity
ligma_eevl_signed_factor (LigmaEevl *eva)
{
  LigmaEevlQuantity result;
  gboolean         negate = FALSE;

  if (! ligma_eevl_accept (eva, '+', NULL))
    negate = ligma_eevl_accept (eva, '-', NULL);

  result = ligma_eevl_factor (eva);

  if (negate) result.value = -result.value;

  return result;
}

static LigmaEevlQuantity
ligma_eevl_factor (LigmaEevl *eva)
{
  LigmaEevlQuantity evaluated_factor;

  evaluated_factor = ligma_eevl_quantity (eva);

  if (ligma_eevl_accept (eva, '^', NULL))
    {
      LigmaEevlQuantity evaluated_exponent;

      evaluated_exponent = ligma_eevl_signed_factor (eva);

      if (evaluated_exponent.dimension != 0)
        ligma_eevl_error (eva, "Exponent is not a dimensionless quantity");

      evaluated_factor.value      = pow (evaluated_factor.value,
                                         evaluated_exponent.value);
      evaluated_factor.dimension *= evaluated_exponent.value;
    }

  return evaluated_factor;
}

static LigmaEevlQuantity
ligma_eevl_quantity (LigmaEevl *eva)
{
  LigmaEevlQuantity evaluated_quantity = { 0, 0 };
  LigmaEevlToken    consumed_token;

  if (ligma_eevl_accept (eva,
                        LIGMA_EEVL_TOKEN_NUM,
                        &consumed_token))
    {
      evaluated_quantity.value = consumed_token.value.fl;
    }
  else if (ligma_eevl_accept (eva, '(', NULL))
    {
      evaluated_quantity = ligma_eevl_expression (eva);
      ligma_eevl_expect (eva, ')', 0);
    }
  else
    {
      ligma_eevl_error (eva, "Expected number or '('");
    }

  if (eva->current_token.type == LIGMA_EEVL_TOKEN_IDENTIFIER)
    {
      gchar            *identifier;
      LigmaEevlQuantity  factor;
      gdouble           offset;

      ligma_eevl_accept (eva,
                        LIGMA_EEVL_TOKEN_ANY,
                        &consumed_token);

      identifier = g_newa (gchar, consumed_token.value.size + 1);

      strncpy (identifier, consumed_token.value.c, consumed_token.value.size);
      identifier[consumed_token.value.size] = '\0';

      if (eva->options.unit_resolver_proc (identifier,
                                           &factor,
                                           &offset,
                                           eva->options.data))
        {
          if (ligma_eevl_accept (eva, '^', NULL))
            {
              LigmaEevlQuantity evaluated_exponent;

              evaluated_exponent = ligma_eevl_signed_factor (eva);

              if (evaluated_exponent.dimension != 0)
                {
                  ligma_eevl_error (eva,
                                   "Exponent is not a dimensionless quantity");
                }

              if (offset != 0.0)
                {
                  ligma_eevl_error (eva,
                                   "Invalid unit exponent");
                }

              factor.value      = pow (factor.value, evaluated_exponent.value);
              factor.dimension *= evaluated_exponent.value;
            }

          evaluated_quantity.value     /= factor.value;
          evaluated_quantity.value     += offset;
          evaluated_quantity.dimension += factor.dimension;
        }
      else
        {
          ligma_eevl_error (eva, "Unit was not resolved");
        }
    }

  return evaluated_quantity;
}

static gboolean
ligma_eevl_accept (LigmaEevl          *eva,
                  LigmaEevlTokenType  token_type,
                  LigmaEevlToken     *consumed_token)
{
  gboolean existed = FALSE;

  if (token_type == eva->current_token.type ||
      token_type == LIGMA_EEVL_TOKEN_ANY)
    {
      existed = TRUE;

      if (consumed_token)
        *consumed_token = eva->current_token;

      /* Parse next token */
      ligma_eevl_lex (eva);
    }

  return existed;
}

static void
ligma_eevl_lex (LigmaEevl *eva)
{
  const gchar *s;

  ligma_eevl_move_past_whitespace (eva);
  s = eva->string;
  eva->start_of_current_token = s;

  if (! s || s[0] == '\0')
    {
      /* We're all done */
      eva->current_token.type = LIGMA_EEVL_TOKEN_END;
    }
  else if (s[0] == '+' || s[0] == '-')
    {
      /* Snatch these before the g_strtod() does, otherwise they might
       * be used in a numeric conversion.
       */
      ligma_eevl_lex_accept_count (eva, 1, s[0]);
    }
  else
    {
      /* Attempt to parse a numeric value */
      gchar  *endptr = NULL;
      gdouble value      = g_strtod (s, &endptr);

      if (endptr && endptr != s)
        {
          /* A numeric could be parsed, use it */
          eva->current_token.value.fl = value;

          ligma_eevl_lex_accept_to (eva, endptr, LIGMA_EEVL_TOKEN_NUM);
        }
      else if (ligma_eevl_unit_identifier_start (s[0]))
        {
          /* Unit identifier */
          eva->current_token.value.c    = s;
          eva->current_token.value.size = ligma_eevl_unit_identifier_size (s, 0);

          ligma_eevl_lex_accept_count (eva,
                                      eva->current_token.value.size,
                                      LIGMA_EEVL_TOKEN_IDENTIFIER);
        }
      else
        {
          /* Everything else is a single character token */
          ligma_eevl_lex_accept_count (eva, 1, s[0]);
        }
    }
}

static void
ligma_eevl_lex_accept_count (LigmaEevl          *eva,
                            gint               count,
                            LigmaEevlTokenType  token_type)
{
  eva->current_token.type  = token_type;
  eva->string             += count;
}

static void
ligma_eevl_lex_accept_to (LigmaEevl          *eva,
                         gchar             *to,
                         LigmaEevlTokenType  token_type)
{
  eva->current_token.type = token_type;
  eva->string             = to;
}

static void
ligma_eevl_move_past_whitespace (LigmaEevl *eva)
{
  if (! eva->string)
    return;

  while (g_ascii_isspace (*eva->string))
    eva->string++;
}

static gboolean
ligma_eevl_unit_identifier_start (gunichar c)
{
  return (g_unichar_isalpha (c) ||
          c == (gunichar) '%'   ||
          c == (gunichar) '\'');
}

static gboolean
ligma_eevl_unit_identifier_continue (gunichar c)
{
  return (ligma_eevl_unit_identifier_start (c) ||
          g_unichar_isdigit (c));
}

/**
 * ligma_eevl_unit_identifier_size:
 * @s:
 * @start:
 *
 * Returns: Size of identifier in bytes (not including NULL
 * terminator).
 **/
static gint
ligma_eevl_unit_identifier_size (const gchar *string,
                                gint         start_offset)
{
  const gchar *start  = g_utf8_offset_to_pointer (string, start_offset);
  const gchar *s      = start;
  gunichar     c      = g_utf8_get_char (s);
  gint         length = 0;

  if (ligma_eevl_unit_identifier_start (c))
    {
      s = g_utf8_next_char (s);
      c = g_utf8_get_char (s);
      length++;

      while (ligma_eevl_unit_identifier_continue (c))
        {
          s = g_utf8_next_char (s);
          c = g_utf8_get_char (s);
          length++;
        }
    }

  return g_utf8_offset_to_pointer (start, length) - start;
}

static void
ligma_eevl_expect (LigmaEevl          *eva,
                  LigmaEevlTokenType  token_type,
                  LigmaEevlToken     *value)
{
  if (! ligma_eevl_accept (eva, token_type, value))
    ligma_eevl_error (eva, "Unexpected token");
}

static void
ligma_eevl_error (LigmaEevl *eva,
                 gchar    *msg)
{
  eva->error_message = msg;
  longjmp (eva->catcher, 1);
}
