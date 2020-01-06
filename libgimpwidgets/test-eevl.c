/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * test-eevl.c
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* A small regression test case for the evaluator */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "gimpeevl.h"


typedef struct
{
  const gchar      *string;
  GimpEevlQuantity  result;
  gboolean          should_succeed;
} TestCase;

static gboolean
test_units (const gchar      *ident,
            GimpEevlQuantity *factor,
            gdouble          *offset,
            gpointer          data)
{
  gboolean resolved     = FALSE;
  gboolean default_unit = (ident == NULL);

  *offset = 0.0;

  if (default_unit ||
      (ident && strcmp ("in", ident) == 0))
    {
      factor->dimension = 1;
      factor->value     = 1.;

      resolved          = TRUE;
    }
  else if (ident && strcmp ("mm", ident) == 0)
    {
      factor->dimension = 1;
      factor->value     = 25.4;

      resolved          = TRUE;
    }

  return resolved;
}


int
main(void)
{
  gint i;
  gint failed    = 0;
  gint succeeded = 0;

  TestCase cases[] =
  {
    /* "Default" test case */
    { "2in + 3in",                 { 2 + 3, 1},               TRUE },

    /* Whitespace variations */
    { "2in+3in",                   { 2 + 3, 1},               TRUE },
    { "   2in + 3in",              { 2 + 3, 1},               TRUE },
    { "2in + 3in   ",              { 2 + 3, 1},               TRUE },
    { "2 in + 3 in",               { 2 + 3, 1},               TRUE },
    { "   2   in   +   3   in   ", { 2 + 3, 1},               TRUE },

    /* Make sure the default unit is applied as it should */
    { "2 + 3in",                   { 2 + 3, 1 },              TRUE },
    { "3",                         { 3, 1 },                  TRUE },

    /* Somewhat complicated input */
    { "(2 + 3)in",                 { 2 + 3, 1},               TRUE },
  //  { "2 / 3 in",                  { 2 / 3., 1},              TRUE },
    { "(2 + 2/3)in",               { 2 + 2 / 3., 1},          TRUE },
    { "1/2 + 1/2",                 { 1, 1},                   TRUE },
    { "2 ^ 3 ^ 4",                 { pow (2, pow (3, 4)), 1}, TRUE },

    /* Mixing of units */
    { "2mm + 3in",                 { 2 / 25.4 + 3, 1},        TRUE },

    /* 'odd' behavior */
    { "2 ++ 1",                    { 3, 1},                   TRUE },
    { "2 +- 1",                    { 1, 1},                   TRUE },
    { "2 -- 1",                    { 3, 1},                   TRUE },

    /* End of test cases */
    { NULL, { 0, 0 }, TRUE }
  };

  g_print ("Testing Eevl Eva, the Evaluator\n\n");

  for (i = 0; cases[i].string; i++)
    {
      const gchar      *test           = cases[i].string;
      GimpEevlOptions   options        = GIMP_EEVL_OPTIONS_INIT;
      GimpEevlQuantity  should         = cases[i].result;
      GimpEevlQuantity  result         = { 0, -1 };
      gboolean          should_succeed = cases[i].should_succeed;
      GError           *error          = NULL;
      const gchar      *error_pos      = 0;

      options.unit_resolver_proc = test_units;

      gimp_eevl_evaluate (test,
                          &options,
                          &result,
                          &error_pos,
                          &error);

      g_print ("%s = %lg (%d): ", test, result.value, result.dimension);
      if (error || error_pos)
        {
          if (should_succeed)
            {
              failed++;
              g_print ("evaluation failed ");
              if (error)
                {
                  g_print ("with: %s, ", error->message);
                }
              else
                {
                  g_print ("without reason, ");
                }
              if (error_pos)
                {
                  if (*error_pos) g_print ("'%s'.", error_pos);
                  else g_print ("at end of input.");
                }
              else
                {
                  g_print ("but didn't say where.");
                }
              g_print ("\n");
            }
          else
            {
              g_print ("OK (failure test case)\n");
              succeeded++;
            }
        }
      else if (!should_succeed)
        {
          g_print ("evaluation should've failed, but didn't.\n");
          failed++;
        }
      else if (should.value != result.value || should.dimension != result.dimension)
        {
          g_print ("results don't match, should be: %lg (%d)\n",
                    should.value, should.dimension);
          failed++;
        }
      else
        {
          g_print ("OK\n");
          succeeded++;
        }
    }

  g_print ("\n");
  if (!failed)
    g_print ("All OK. ");
  else
    g_print ("Test failed! ");

  g_print ("(%d/%d) %lg%%\n\n", succeeded, succeeded+failed,
            100*succeeded/(gdouble)(succeeded+failed));

  return failed;
}
