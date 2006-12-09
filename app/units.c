/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase-private.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpunit.h"

#include "units.h"


static Gimp *the_unit_gimp = NULL;


static gint
units_get_number_of_units (void)
{
  return _gimp_unit_get_number_of_units (the_unit_gimp);
}

static gint
units_get_number_of_built_in_units (void)
{
  return GIMP_UNIT_END;
}

static GimpUnit
units_unit_new (gchar   *identifier,
                gdouble  factor,
                gint     digits,
                gchar   *symbol,
                gchar   *abbreviation,
                gchar   *singular,
                gchar   *plural)
{
  return _gimp_unit_new (the_unit_gimp,
                         identifier,
                         factor,
                         digits,
                         symbol,
                         abbreviation,
                         singular,
                         plural);
}

static gboolean
units_unit_get_deletion_flag (GimpUnit unit)
{
  return _gimp_unit_get_deletion_flag (the_unit_gimp, unit);
}

static void
units_unit_set_deletion_flag (GimpUnit unit,
                              gboolean deletion_flag)
{
  _gimp_unit_set_deletion_flag (the_unit_gimp, unit, deletion_flag);
}

static gdouble
units_unit_get_factor (GimpUnit unit)
{
  return _gimp_unit_get_factor (the_unit_gimp, unit);
}

static gint
units_unit_get_digits (GimpUnit unit)
{
  return _gimp_unit_get_digits (the_unit_gimp, unit);
}

static const gchar *
units_unit_get_identifier (GimpUnit unit)
{
  return _gimp_unit_get_identifier (the_unit_gimp, unit);
}

static const gchar *
units_unit_get_symbol (GimpUnit unit)
{
  return _gimp_unit_get_symbol (the_unit_gimp, unit);
}

static const gchar *
units_unit_get_abbreviation (GimpUnit unit)
{
  return _gimp_unit_get_abbreviation (the_unit_gimp, unit);
}

static const gchar *
units_unit_get_singular (GimpUnit unit)
{
  return _gimp_unit_get_singular (the_unit_gimp, unit);
}

static const gchar *
units_unit_get_plural (GimpUnit unit)
{
  return _gimp_unit_get_plural (the_unit_gimp, unit);
}

void
units_init (Gimp *gimp)
{
  GimpUnitVtable vtable;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (the_unit_gimp == NULL);

  the_unit_gimp = gimp;

  vtable.unit_get_number_of_units          = units_get_number_of_units;
  vtable.unit_get_number_of_built_in_units = units_get_number_of_built_in_units;
  vtable.unit_new               = units_unit_new;
  vtable.unit_get_deletion_flag = units_unit_get_deletion_flag;
  vtable.unit_set_deletion_flag = units_unit_set_deletion_flag;
  vtable.unit_get_factor        = units_unit_get_factor;
  vtable.unit_get_digits        = units_unit_get_digits;
  vtable.unit_get_identifier    = units_unit_get_identifier;
  vtable.unit_get_symbol        = units_unit_get_symbol;
  vtable.unit_get_abbreviation  = units_unit_get_abbreviation;
  vtable.unit_get_singular      = units_unit_get_singular;
  vtable.unit_get_plural        = units_unit_get_plural;

  gimp_base_init (&vtable);
}
