/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunit_pdb.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

#include "gimp.h"


gint
_gimp_unit_get_number_of_units (void)
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

gint
_gimp_unit_get_number_of_built_in_units (void)
{
  GParam *return_vals;
  gint nreturn_vals;

  gint number;

  return_vals = gimp_run_procedure ("gimp_unit_get_number_of_built_in_units",
				    &nreturn_vals,
				    PARAM_END);

  number = GIMP_UNIT_END;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    number = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return number;
}

GimpUnit
_gimp_unit_new (gchar   *identifier,
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
				    PARAM_STRING, identifier,
				    PARAM_FLOAT,  factor,
				    PARAM_INT32,  digits,
				    PARAM_STRING, symbol,
				    PARAM_STRING, abbreviation,
				    PARAM_STRING, singular,
				    PARAM_STRING, plural,
				    PARAM_END);

  unit = GIMP_UNIT_INCH;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    unit = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return unit;
}

gboolean
_gimp_unit_get_deletion_flag (GimpUnit unit)
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

void
_gimp_unit_set_deletion_flag (GimpUnit unit,
			      gboolean deletion_flag)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_unit_set_deletion_flag",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_INT32, deletion_flag,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gdouble
_gimp_unit_get_factor (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gdouble factor;

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
_gimp_unit_get_digits (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gint digits;

  return_vals = gimp_run_procedure ("gimp_unit_get_digits",
				    &nreturn_vals,
				    PARAM_INT32, unit,
				    PARAM_END);

  digits = 2; /* == gimp_unit_defs[GIMP_UNIT_INCH].digits */
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    digits = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return digits;
}

gchar * 
_gimp_unit_get_identifier (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *identifier;

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
_gimp_unit_get_symbol (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *symbol;

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
_gimp_unit_get_abbreviation (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *abbreviation;

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
_gimp_unit_get_singular (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *singular;

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
_gimp_unit_get_plural (GimpUnit unit)
{
  GParam *return_vals;
  gint nreturn_vals;

  gchar *plural;

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
