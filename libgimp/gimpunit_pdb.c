/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpunit.c
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gimp.h"
#include "gimpunit.h"

#include "libgimp-intl.h"

/*  internal structures  */

typedef struct
{
  gboolean  delete_on_exit;
  gdouble   factor;
  gint      digits;
  gchar    *identifier;
  gchar    *symbol;
  gchar    *abbreviation;
  gchar    *singular;
  gchar    *plural;
} GimpUnitDef;

static GimpUnitDef gimp_unit_defs[GIMP_UNIT_END] =
{
  /* pseudo unit */
  { FALSE,  0.0, 0, "pixels",      "px", "px", N_("pixel"),      N_("pixels") },

  /* standard units */
  { FALSE,  1.0, 2, "inches",      "''", "in", N_("inch"),       N_("inches") },
  { FALSE, 25.4, 1, "millimeters", "mm", "mm", N_("millimeter"), N_("millimeters") },

  /* professional units */
  { FALSE, 72.0, 0, "points",      "pt", "pt", N_("point"),      N_("points") },
  { FALSE,  6.0, 1, "picas",       "pc", "pc", N_("pica"),       N_("picas") },
};

/*  not a unit at all but kept here to have the strings in one place
 */
static GimpUnitDef gimp_unit_percent =
{
  FALSE,    0.0, 0, "percent",     "%",  "%",  N_("percent"),    N_("percent")
};


/**
 * gimp_unit_get_number_of_units:
 *
 * Returns the number of units which are known to the #GimpUnit system.
 *
 * Returns: The number of defined units.
 *
 */
gint
gimp_unit_get_number_of_units (void)
{
  GParam *return_vals;
  gint nreturn_vals;

  gint number;

  return_vals = gimp_run_procedure ("gimp_unit_get_number_of_units",
				    &nreturn_vals,
				    PARAM_END);

  number = GIMP_UNIT_END;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    number = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return number;
}

/**
 * gimp_unit_get_number_of_built_in_units:
 *
 * Returns the number of #GimpUnit's which are hardcoded in the unit system
 * (UNIT_INCH, UNIT_MM, UNIT_POINT, UNIT_PICA and the two "pseudo units"
 *  UNIT_PIXEL and UNIT_PERCENT).
 *
 * Returns: The number of built-in units.
 *
 */
gint
gimp_unit_get_number_of_built_in_units (void)
{
  return GIMP_UNIT_END;
}

/**
 * gimp_unit_new:
 * @identifier: The unit's identifier string.
 * @factor: The unit's factor (how many units are in one inch).
 * @digits: The unit's suggested number of digits (see gimp_unit_get_digits()).
 * @symbol: The symbol of the unit (e.g. "''" for inch).
 * @abbreviation: The abbreviation of the unit.
 * @singular: The singular form of the unit.
 * @plural: The plural form of the unit.
 *
 * Returns the integer ID of the new #GimpUnit.
 *
 * Note that a new unit is always created with it's deletion flag
 * set to #TRUE. You will have to set it to #FALSE with
 * gimp_unit_set_deletion_flag() to make the unit definition persistent.
 *
 * Returns: The ID of the new unit.
 *
 */
GimpUnit
gimp_unit_new (gchar   *identifier,
	       gdouble  factor,
	       gint     digits,
	       gchar   *symbol,
	       gchar   *abbreviation,
	       gchar   *singular,
	       gchar   *plural)
{
  GParam *return_vals;
  gint nreturn_vals;

  GimpUnit unit;

  return_vals = gimp_run_procedure ("gimp_unit_new",
				    &nreturn_vals,
				    PARAM_STRING, g_strdup (identifier),
				    PARAM_FLOAT,  factor,
				    PARAM_INT32,  digits,
				    PARAM_STRING, g_strdup (symbol),
				    PARAM_STRING, g_strdup (abbreviation),
				    PARAM_STRING, g_strdup (singular),
				    PARAM_STRING, g_strdup (plural),
				    PARAM_END);

  unit = GIMP_UNIT_INCH;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    unit = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return unit;
}

/**
 * gimp_unit_get_deletion_flag:
 * @unit: The unit you want to know the @deletion_flag of.
 *
 * Returns: The unit's @deletion_flag.
 *
 */
gboolean
gimp_unit_get_deletion_flag (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gboolean flag;

  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, TRUE);

  if (unit < GIMP_UNIT_END)
    return FALSE;

  return_vals = gimp_run_procedure ("gimp_unit_get_deletion_flag",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  flag = TRUE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    flag = return_vals[1].data.d_int32 ? TRUE : FALSE;

  gimp_destroy_params (return_vals, nreturn_vals);

  return flag;
}

/**
 * gimp_unit_set_deletion_flag:
 * @unit: The unit you want to set the @deletion_flag for.
 * @deletion_flag: The new deletion_flag.
 *
 * Sets a #GimpUnit's @deletion_flag. If the @deletion_flag of a unit is
 * #TRUE when GIMP exits, this unit will not be saved in the uses's
 * "unitrc" file.
 *
 * Trying to change the @deletion_flag of a built-in unit will be silently
 * ignored.
 *
 */
void
gimp_unit_set_deletion_flag (GimpUnit unit,
			     gboolean deletion_flag)
{
  GParam *return_vals;
  gint nreturn_vals;

  g_return_if_fail (unit >= GIMP_UNIT_PIXEL);

  if (unit < GIMP_UNIT_END)
    return;

  return_vals = gimp_run_procedure ("gimp_unit_set_deletion_flag",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_INT32, deletion_flag,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

/**
 * gimp_unit_get_factor:
 * @unit: The unit you want to know the factor of.
 *
 * A #GimpUnit's @factor is defined to be:
 *
 * distance_in_units == (@factor * distance_in_inches)
 *
 * Returns 0 for @unit == GIMP_UNIT_PIXEL.
 *
 * Returns: The unit's factor.
 *
 */
gdouble
gimp_unit_get_factor (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gdouble factor;

  g_return_val_if_fail (unit >= GIMP_UNIT_INCH, 1.0);

  if (unit < GIMP_UNIT_END)
    return gimp_unit_defs[unit].factor;

  return_vals = gimp_run_procedure ("gimp_unit_get_factor",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  factor = 1.0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    factor = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return factor;
}

/**
 * gimp_unit_get_digits:
 * @unit: The unit you want to know the digits.
 *
 * Returns the number of digits an entry field should provide to get
 * approximately the same accuracy as an inch input field with two digits.
 *
 * Returns 0 for @unit == GIMP_UNIT_PIXEL.
 *
 * Returns: The suggested number of digits.
 *
 */
gint
gimp_unit_get_digits (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gint digits;

  if (unit < 0)
    return 0;

  if (unit < GIMP_UNIT_END)
    return gimp_unit_defs[unit].digits;

  return_vals = gimp_run_procedure ("gimp_unit_get_digits",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  digits = gimp_unit_defs[GIMP_UNIT_INCH].digits;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    digits = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return digits;
}

/**
 * gimp_unit_get_identifier:
 * @unit: The unit you want to know the identifier of.
 *
 * This is an unstranslated string.
 *
 * NOTE: This string has to be g_free()'d by plugins but is a pointer to a
 *       constant string when this function is used from inside the GIMP.
 *
 * Returns: The unit's identifier.
 *
 */
gchar * 
gimp_unit_get_identifier (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *identifier;

  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, g_strdup (""));

  if (unit < GIMP_UNIT_END)
    return g_strdup (gimp_unit_defs[unit].identifier);

  if (unit == GIMP_UNIT_PERCENT)
    return g_strdup (gimp_unit_percent.identifier);

  return_vals = gimp_run_procedure ("gimp_unit_get_identifier",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  identifier = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    identifier = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return identifier ? identifier : g_strdup ("");
}

/**
 * gimp_unit_get_symbol:
 * @unit: The unit you want to know the symbol of.
 *
 * This is e.g. "''" for UNIT_INCH.
 *
 * NOTE: This string has to be g_free()'d by plugins but is a pointer to a
 *       constant string when this function is used from inside the GIMP.
 *
 * Returns: The unit's symbol.
 *
 */
gchar *
gimp_unit_get_symbol (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *symbol;

  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, g_strdup (""));

  if (unit < GIMP_UNIT_END)
    return g_strdup (gimp_unit_defs[unit].symbol);

  if (unit == GIMP_UNIT_PERCENT)
    return g_strdup (gimp_unit_percent.symbol);

  return_vals = gimp_run_procedure ("gimp_unit_get_symbol",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  symbol = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    symbol = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return symbol ? symbol : g_strdup ("");
}


/**
 * gimp_unit_get_abbreviation:
 * @unit: The unit you want to know the abbreviation of.
 *
 * For built-in units, this function returns the translated abbreviation
 * of the unit.
 *
 * NOTE: This string has to be g_free()'d by plugins but is a pointer to a
 *       constant string when this function is used from inside the GIMP.
 *
 * Returns: The unit's abbreviation.
 *
 */
gchar *
gimp_unit_get_abbreviation (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *abbreviation;

  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, g_strdup (""));

  if (unit < GIMP_UNIT_END)
    return g_strdup (gimp_unit_defs[unit].abbreviation);

  if (unit == GIMP_UNIT_PERCENT)
    return g_strdup (gimp_unit_percent.abbreviation);

  return_vals = gimp_run_procedure ("gimp_unit_get_abbreviation",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  abbreviation = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    abbreviation = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return abbreviation ? abbreviation : g_strdup ("");
}


/**
 * gimp_unit_get_singular:
 * @unit: The unit you want to know the singular form of.
 *
 * For built-in units, this function returns the translated singular form
 * of the unit's name.
 *
 * NOTE: This string has to be g_free()'d by plugins but is a pointer to a
 *       constant string when this function is used from inside the GIMP.
 *
 * Returns: The unit's singular form.
 *
 */
gchar *
gimp_unit_get_singular (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *singular;

  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, g_strdup (""));

  if (unit < GIMP_UNIT_END)
    return g_strdup (gettext (gimp_unit_defs[unit].singular));

  if (unit == GIMP_UNIT_PERCENT)
    return g_strdup (gettext (gimp_unit_percent.singular));

  return_vals = gimp_run_procedure ("gimp_unit_get_singular",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  singular = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    singular = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return singular ? singular : g_strdup ("");
}

/**
 * gimp_unit_get_plural:
 * @unit: The unit you want to know the plural form of.
 *
 * For built-in units, this function returns the translated plural form
 * of the unit's name.
 *
 * NOTE: This string has to be g_free()'d by plugins but is a pointer to a
 *       constant string when this function is used from inside the GIMP.
 *
 * Returns: The unit's plural form.
 *
 */
gchar *
gimp_unit_get_plural (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *plural;

  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, g_strdup (""));

  if (unit < GIMP_UNIT_END)
    return g_strdup (gettext (gimp_unit_defs[unit].plural));

  if (unit == GIMP_UNIT_PERCENT)
    return g_strdup (gettext (gimp_unit_percent.plural));

  return_vals = gimp_run_procedure ("gimp_unit_get_plural",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  plural = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    plural = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return plural ? plural : g_strdup ("");
}
