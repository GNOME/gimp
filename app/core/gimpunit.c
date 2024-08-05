/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpunit.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

/* This file contains the definition of the size unit objects. The
 * factor of the units is relative to inches (which have a factor of 1).
 */

#include "config.h"

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpunit.h"

#include "gimp-intl.h"


/* public functions */

GimpUnit *
_gimp_unit_new (Gimp        *gimp,
                const gchar *name,
                gdouble      factor,
                gint         digits,
                const gchar *symbol,
                const gchar *abbreviation)
{
  GimpUnit *unit;
  gint      unit_id;

  unit_id = GIMP_UNIT_END + g_list_length (gimp->user_units);
  unit = g_object_new (GIMP_TYPE_UNIT,
                       "id",           unit_id,
                       "name",         name,
                       "factor",       factor,
                       "digits",       digits,
                       "symbol",       symbol,
                       "abbreviation", abbreviation,
                       NULL);

  gimp->user_units = g_list_append (gimp->user_units, unit);
  gimp_unit_set_deletion_flag (unit, TRUE);

  return unit;
}
