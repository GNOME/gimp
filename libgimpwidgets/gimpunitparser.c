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
 * prototypes
 */

/* unit resolver for GimpEevl */
static gboolean unit_resolver (const gchar      *ident,
                               GimpEevlQuantity *result,
                               gpointer          data);


/* parse given string */
gboolean 
gimp_unit_parser_parse (const char *str, GimpUnitParserResult *result)
{
  /* GimpEevl related stuff */
  GimpEevlQuantity  eevlResult;
  GError            *error    = NULL;
  const gchar       *errorpos = 0;

  if (strlen (str) <= 0)
    return FALSE;
  
  /* set unit to -1 so we can determine the first unit the user entered and use that
     as unit for our result */
  result->unit = -1;

  DEBUG (("parsing: %s", str));

  /* parse text via GimpEevl */
  gimp_eevl_evaluate (str,
                      unit_resolver,
                      &eevlResult,
                      (gpointer) result,
                      &errorpos,
                      &error);

  if (error || errorpos)
  {
    DEBUG (("gimpeevl parsing error \n"));
    return FALSE;
  }
  else
  {
    DEBUG (("gimpeevl parser result: %s = %lg (%d)", str, eevlResult.value, eevlResult.dimension));
    DEBUG (("determined unit: %s\n", gimp_unit_get_abbreviation (result->unit)));

    result->value = eevlResult.value;
  }

  return TRUE;
}

/* unit resolver for GimpEevl */
static 
gboolean unit_resolver (const gchar      *ident,
                        GimpEevlQuantity *result,
                        gpointer          user_data)
{
  GimpUnitParserResult   *parserResult = (GimpUnitParserResult*) user_data;
  GimpUnit               *unit        = &(parserResult->unit);
  gboolean               resolved     = FALSE;
  gboolean               default_unit = (ident == NULL);
  gint                   numUnits     = gimp_unit_get_number_of_units ();
  const gchar            *abbr; 
  gint                   i            = 0;

  result->dimension = 1;

  /* if no unit is specified, use default unit */
  if (default_unit)
  {
    /* if default hasn't been set before, set to inch*/
    if (*unit == -1)
      *unit = GIMP_UNIT_INCH;

    result->dimension = 1;

    if (*unit == GIMP_UNIT_PIXEL) /* handle case that unit is px */
      result->value = parserResult->resolution;
    else                          /* otherwise use factor */
      result->value = gimp_unit_get_factor (*unit);

    resolved          = TRUE; 
    return resolved;
  }

  /* find matching unit */
  for (i = 0; i < numUnits; i++)
  {
    abbr = gimp_unit_get_abbreviation (i);

    if (strcmp (abbr, ident) == 0)
    {
      /* handle case that unit is px */
      if (i == GIMP_UNIT_PIXEL)
        result->value = parserResult->resolution;
      else
        result->value = gimp_unit_get_factor (i);

      if (*unit == -1)
        *unit = i;

      i = numUnits;
      resolved = TRUE;
    }
  }

  return resolved;
}
                              