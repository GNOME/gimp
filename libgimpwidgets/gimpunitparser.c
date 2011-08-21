/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitparser.c
 * Copyright (C) 2011 Enrico Schröder <enni.schroeder@gmail.com>
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
 * <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "gimpunitparser.h"
#include "gimpeevl.h"

//#define PARSER_DEBUG
#ifdef  PARSER_DEBUG
#define DEBUG(x) g_debug x 
#else
#define DEBUG(x) /* nothing */
#endif

/**
 * SECTION: gimpunitparser
 * @title: GimpUnitParser
 * @short_description: A wrapper around #GimpEevl providing unit/value parsing of strings.
 * @see_also: ##GimpUnitEntry, #GimpEevl
 *
 * Provides the function to parse a string containing values in one or more units and determines
 * the resulting value and unit. The input is parsed via the #GimpEevl
 * parser, supporting basic mathematical operations including terms with different
 * units (e.g. "20 cm + 10 px"). 
 *
 * It is used by #GimpUnitEntry.
 **/

/* unit resolver for GimpEevl */
static gboolean unit_resolver (const gchar      *ident,
                               GimpEevlQuantity *result,
                               gpointer          data);


/**
 * gimp_unit_parser_parse:
 * @str:    The string to parse.
 * @result: Pointer to a #GimpUnitParserResult in which the results are written.
 *
 * Parses the string via #GimpEevl and calculates the value and unit of it. The results
 * are written in the passed #GimpUnitParserResult struct.
 *
 * Before you call the function, you need to fill the fields "resolution" and "unit" of the 
 * GimpUnitParserResult struct in order for the calculations to be correct.
 * 
 * The field "resolution" must contain the resolution of the value in dpi. It is used to
 * calculate the correct value if the user enters pixels.
 *
 * The field "unit" must contain the unit to use in case the user just enters a value without
 * any unit.
 *
 * After the function returns, the field "value" contains the result value of the string and
 * "unit" the determined unit. The unit of the string will be either the first entered or the
 * given unit if the user didn't enter any.
 *
 *<example>
 *  <title>Using GimpUnitParser.</title>
 *    <programlisting>
 *    gboolean             success;
 *    GimpUnitParserResult result;
 *    const gchar          *string = foo_get_some_input_string();
 *
 *    // set resolution (important for correct calculation of px)
 *    result.resolution = 72.0;
 *    // set unit (we want to use pixels if the user didn't enter one) 
 *    result.unit = GIMP_UNIT_PIXELS;
 *    // parse string
 *    success = gimp_unit_parser_parse (string, &result);
 *
 *    // result now contains the value and unit of the string
 *    </programlisting>
 *</example>
 *
 * Returns: TRUE if the string was correctly parsed, FALSE if it was invalid
 **/

gboolean 
gimp_unit_parser_parse (const char *str, GimpUnitParserResult *result)
{
  /* GimpEevl related stuff */
  GimpEevlQuantity  eevl_result;
  GError            *error    = NULL;
  const gchar       *errorpos = 0;

  if (strlen (str) <= 0)
    return FALSE;
    
  /* set unit_found to FALSE so we can determine the first unit the user entered and use that
     as unit for our result */
  result->unit_found = FALSE;

  DEBUG (("parsing: %s", str));

  /* parse text via GimpEevl */
  gimp_eevl_evaluate (str,
                      unit_resolver,
                      &eevl_result,
                      (gpointer) result,
                      &errorpos,
                      &error);

  if (error || errorpos || eevl_result.dimension > 1)
  {
    DEBUG (("gimpeevl parsing error \n"));
    return FALSE;
  }
  else
  {
    DEBUG (("gimpeevl parser result: %s = %lg (%d)", str, eevl_result.value, eevl_result.dimension));
    DEBUG (("determined unit: %s\n", gimp_unit_get_abbreviation (result->unit)));

    result->value = eevl_result.value;
  }

  return TRUE;
}

/* unit resolver for GimpEevl */
static 
gboolean unit_resolver (const gchar      *ident,
                        GimpEevlQuantity *result,
                        gpointer          user_data)
{
  GimpUnitParserResult   *parser_result = (GimpUnitParserResult*) user_data;
  GimpUnit               *unit          = &(parser_result->unit);
  gboolean               resolved       = FALSE;
  gboolean               default_unit   = (ident == NULL);
  gint                   num_units      = gimp_unit_get_number_of_units ();
  const gchar            *abbr; 
  gint                   i              = 0;

  result->dimension = 1;
  DEBUG (("unit resolver: %s", ident));

  /* if no unit is specified, use default unit */
  if (default_unit)
  {
    /* if default hasn't been set before, set to inch*/
    if (*unit == -1)
      *unit = GIMP_UNIT_INCH;

    result->dimension = 1;

    if (*unit == GIMP_UNIT_PIXEL) /* handle case that unit is px */
      result->value = parser_result->resolution;
    else                          /* otherwise use unit factor */
      result->value = gimp_unit_get_factor (*unit);

    parser_result->unit_found = TRUE;
    resolved                  = TRUE; 
    
    return resolved;
  }

  /* find matching unit */
  for (i = 0; i < num_units; i++)
  {
    abbr = gimp_unit_get_abbreviation (i);

    if (strcmp (abbr, ident) == 0)
    {
      /* handle case that unit is px */
      if (i == GIMP_UNIT_PIXEL)
        result->value = parser_result->resolution;
      else
        result->value = gimp_unit_get_factor (i);

      if (!parser_result->unit_found)
      {
        *unit = i;
        parser_result->unit_found = TRUE;
      }

      i = num_units;
      resolved = TRUE;
    }
  }

  return resolved;
}
                              