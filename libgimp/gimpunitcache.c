/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpunit.c
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "gimp.h"
#include "gimpunit.h"
#include "libgimp/gimpintl.h"

/*  internal structures  */

typedef struct {
  guint    delete_on_exit;
  gdouble  factor;
  gint     digits;
  gchar   *identifier;
  gchar   *symbol;
  gchar   *abbreviation;
  gchar   *singular;
  gchar   *plural;
} GimpUnitDef;

static GimpUnitDef gimp_unit_defs[UNIT_END] =
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

/*  public functions  */

gint
gimp_unit_get_number_of_units (void)
{
  GParam *return_vals;
  int nreturn_vals;

  int number;

  return_vals = gimp_run_procedure ("gimp_unit_get_number_of_units",
				    &nreturn_vals,
				    PARAM_END);

  number = UNIT_END;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    number = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return number;
}

gint
gimp_unit_get_number_of_built_in_units (void)
{
  return UNIT_END;
}


GUnit
gimp_unit_new (gchar   *identifier,
	       gdouble  factor,
	       gint     digits,
	       gchar   *symbol,
	       gchar   *abbreviation,
	       gchar   *singular,
	       gchar   *plural)
{
  GParam *return_vals;
  int nreturn_vals;

  GUnit unit;

  return_vals = gimp_run_procedure ("gimp_unit_new",
				    &nreturn_vals,
				    PARAM_STRING, g_strdup (identifier),
				    PARAM_FLOAT, factor,
				    PARAM_INT32, digits,
				    PARAM_STRING, g_strdup (symbol),
				    PARAM_STRING, g_strdup (abbreviation),
				    PARAM_STRING, g_strdup (singular),
				    PARAM_STRING, g_strdup (plural),
				    PARAM_END);

  unit = UNIT_INCH;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    unit = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return unit;
}


guint
gimp_unit_get_deletion_flag (GUnit unit)
{
  GParam *return_vals;
  int nreturn_vals;

  guint flag;

  g_return_val_if_fail (unit >= UNIT_PIXEL, TRUE);

  if (unit < UNIT_END)
    return FALSE;

  return_vals = gimp_run_procedure ("gimp_unit_get_deletion_flag",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  flag = TRUE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    flag = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return flag;
}

void
gimp_unit_set_deletion_flag (GUnit  unit,
			     guint  deletion_flag)
{
  GParam *return_vals;
  int nreturn_vals;

  g_return_if_fail (unit >= UNIT_PIXEL);

  if (unit < UNIT_END)
    return;

  return_vals = gimp_run_procedure ("gimp_unit_set_deletion_flag",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_INT32, deletion_flag,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}


gdouble
gimp_unit_get_factor (GUnit unit)
{
  GParam *return_vals;
  int nreturn_vals;

  gdouble factor;

  g_return_val_if_fail (unit >= UNIT_INCH, 1.0);

  if (unit < UNIT_END)
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


gint
gimp_unit_get_digits (GUnit unit)
{
  GParam *return_vals;
  int nreturn_vals;

  gint digits;

  if (unit < 0)
    return 0;

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].digits;

  return_vals = gimp_run_procedure ("gimp_unit_get_digits",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  digits = gimp_unit_defs[UNIT_INCH].digits;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    digits = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return digits;
}


gchar * 
gimp_unit_get_identifier (GUnit unit)
{
  GParam *return_vals;
  int nreturn_vals;

  gchar *identifier;

  g_return_val_if_fail (unit >= UNIT_PIXEL, g_strdup (""));

  if (unit < UNIT_END)
    return g_strdup (gimp_unit_defs[unit].identifier);

  if (unit == UNIT_PERCENT)
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


gchar *
gimp_unit_get_symbol (GUnit unit)
{
  GParam *return_vals;
  int nreturn_vals;

  gchar *symbol;

  g_return_val_if_fail (unit >= UNIT_PIXEL, g_strdup (""));

  if (unit < UNIT_END)
    return g_strdup (gimp_unit_defs[unit].symbol);

  if (unit == UNIT_PERCENT)
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


gchar *
gimp_unit_get_abbreviation (GUnit unit)
{
  GParam *return_vals;
  int nreturn_vals;

  gchar *abbreviation;

  g_return_val_if_fail (unit >= UNIT_PIXEL, g_strdup (""));

  if (unit < UNIT_END)
    return g_strdup (gimp_unit_defs[unit].abbreviation);

  if (unit == UNIT_PERCENT)
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


gchar *
gimp_unit_get_singular (GUnit unit)
{
  GParam *return_vals;
  int nreturn_vals;

  gchar *singular;

  g_return_val_if_fail (unit >= UNIT_PIXEL, g_strdup (""));

  if (unit < UNIT_END)
    return g_strdup (gettext (gimp_unit_defs[unit].singular));

  if (unit == UNIT_PERCENT)
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


gchar *
gimp_unit_get_plural (GUnit unit)
{
  GParam *return_vals;
  int nreturn_vals;

  gchar *plural;

  g_return_val_if_fail (unit >= UNIT_PIXEL, g_strdup (""));

  if (unit < UNIT_END)
    return g_strdup (gettext (gimp_unit_defs[unit].plural));

  if (unit == UNIT_PERCENT)
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
