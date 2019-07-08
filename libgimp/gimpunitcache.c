/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitcache.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "gimpunitcache.h"
#include "gimpunit_pdb.h"

#include "libgimp-intl.h"

/*  internal structures  */

typedef struct
{
  gdouble      factor;
  gint         digits;
  const gchar *identifier;
  const gchar *symbol;
  const gchar *abbreviation;
  const gchar *singular;
  const gchar *plural;
} GimpUnitDef;


static GimpUnitDef * gimp_unit_defs         = NULL;
static GimpUnit      gimp_units_initialized = 0;

/*  not a unit at all but kept here to have the strings in one place
 */
static const GimpUnitDef gimp_unit_percent =
{
  0.0, 0, "percent", "%", "%",  N_("percent"), N_("percent")
};


static void  gimp_unit_def_init (GimpUnitDef *unit_def,
                                 GimpUnit     unit);


static gboolean
gimp_unit_init (GimpUnit unit)
{
  gint i, n;

  if (unit < gimp_units_initialized)
    return TRUE;

  n = _gimp_unit_get_number_of_units ();

  if (unit >= n)
    return FALSE;

  gimp_unit_defs = g_renew (GimpUnitDef, gimp_unit_defs, n);

  for (i = gimp_units_initialized; i < n; i++)
    {
      gimp_unit_def_init (&gimp_unit_defs[i], i);
    }

  gimp_units_initialized = n;

  return TRUE;
}

static void
gimp_unit_def_init (GimpUnitDef *unit_def,
                    GimpUnit     unit)
{
  unit_def->factor       = _gimp_unit_get_factor (unit);
  unit_def->digits       = _gimp_unit_get_digits (unit);
  unit_def->identifier   = _gimp_unit_get_identifier (unit);
  unit_def->symbol       = _gimp_unit_get_symbol (unit);
  unit_def->abbreviation = _gimp_unit_get_abbreviation (unit);
  unit_def->singular     = _gimp_unit_get_singular (unit);
  unit_def->plural       = _gimp_unit_get_plural (unit);
}

gint
_gimp_unit_cache_get_number_of_units (void)
{
  return _gimp_unit_get_number_of_units ();
}

gint
_gimp_unit_cache_get_number_of_built_in_units (void)
{
  return GIMP_UNIT_END;
}

GimpUnit
_gimp_unit_cache_new (gchar   *identifier,
                      gdouble  factor,
                      gint     digits,
                      gchar   *symbol,
                      gchar   *abbreviation,
                      gchar   *singular,
                      gchar   *plural)
{
  return _gimp_unit_new (identifier,
                         factor,
                         digits,
                         symbol,
                         abbreviation,
                         singular,
                         plural);
}

gboolean
_gimp_unit_cache_get_deletion_flag (GimpUnit unit)
{
  if (unit < GIMP_UNIT_END)
    return FALSE;

  return _gimp_unit_get_deletion_flag (unit);
}

void
_gimp_unit_cache_set_deletion_flag (GimpUnit unit,
                                    gboolean deletion_flag)
{
  if (unit < GIMP_UNIT_END)
    return;

  _gimp_unit_set_deletion_flag (unit,
                                deletion_flag);
}

gdouble
_gimp_unit_cache_get_factor (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_INCH, 1.0);

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.factor;

  if (!gimp_unit_init (unit))
    return 1.0;

  return gimp_unit_defs[unit].factor;
}

gint
_gimp_unit_cache_get_digits (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_INCH, 0);

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.digits;

  if (!gimp_unit_init (unit))
    return 0;

  return gimp_unit_defs[unit].digits;
}

const gchar *
_gimp_unit_cache_get_identifier (GimpUnit unit)
{
  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.identifier;

  if (!gimp_unit_init (unit))
    return NULL;

  return gimp_unit_defs[unit].identifier;
}

const gchar *
_gimp_unit_cache_get_symbol (GimpUnit unit)
{
  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.symbol;

  if (!gimp_unit_init (unit))
    return NULL;

  return gimp_unit_defs[unit].symbol;
}

const gchar *
_gimp_unit_cache_get_abbreviation (GimpUnit unit)
{
  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.abbreviation;

  if (!gimp_unit_init (unit))
    return NULL;

  return gimp_unit_defs[unit].abbreviation;
}

const gchar *
_gimp_unit_cache_get_singular (GimpUnit unit)
{
  if (unit == GIMP_UNIT_PERCENT)
    return gettext (gimp_unit_percent.singular);

  if (!gimp_unit_init (unit))
    return NULL;

  return gettext (gimp_unit_defs[unit].singular);
}

const gchar *
_gimp_unit_cache_get_plural (GimpUnit unit)
{
  if (unit == GIMP_UNIT_PERCENT)
    return gettext (gimp_unit_percent.plural);

  if (!gimp_unit_init (unit))
    return NULL;

  return gettext (gimp_unit_defs[unit].plural);
}
