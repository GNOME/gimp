/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunit.c
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

#include "config.h"

#include <glib.h>

#include "libgimpbase/gimpbasetypes.h"

#include "libgimpbase/gimpunit.h"

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
static GimpUnitDef gimp_unit_percent =
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

/**
 * gimp_unit_get_number_of_units:
 *
 * Returns the number of units which are known to the #GimpUnit system.
 *
 * Returns: The number of defined units.
 */
gint
gimp_unit_get_number_of_units (void)
{
  return _gimp_unit_get_number_of_units ();
}

/**
 * gimp_unit_get_number_of_built_in_units:
 *
 * Returns the number of #GimpUnit's which are hardcoded in the unit system
 * (UNIT_INCH, UNIT_MM, UNIT_POINT, UNIT_PICA and the two "pseudo unit"
 *  UNIT_PIXEL).
 *
 * Returns: The number of built-in units.
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
  return _gimp_unit_new (identifier,
			 factor,
			 digits,
			 symbol,
			 abbreviation,
			 singular,
			 plural);
}

/**
 * gimp_unit_get_deletion_flag:
 * @unit: The unit you want to know the @deletion_flag of.
 *
 * Returns: The unit's @deletion_flag.
 */
gboolean
gimp_unit_get_deletion_flag (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, TRUE);

  if (unit < GIMP_UNIT_END)
    return FALSE;

  return _gimp_unit_get_deletion_flag (unit);
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
 */
void
gimp_unit_set_deletion_flag (GimpUnit unit,
			     gboolean deletion_flag)
{
  g_return_if_fail (unit >= GIMP_UNIT_PIXEL);

  if (unit < GIMP_UNIT_END)
    return;

  _gimp_unit_set_deletion_flag (unit,
				deletion_flag);
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
 */
gdouble
gimp_unit_get_factor (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_INCH, 1.0);

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.factor;

  if (!gimp_unit_init (unit))
    return 1.0;

  return gimp_unit_defs[unit].factor;
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
 */
gint
gimp_unit_get_digits (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_INCH, 0);

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.digits;

  if (!gimp_unit_init (unit))
    return 0;

  return gimp_unit_defs[unit].digits;
}

/**
 * gimp_unit_get_identifier:
 * @unit: The unit you want to know the identifier of.
 *
 * This is an unstranslated string and must not be changed or freed.
 *
 * Returns: The unit's identifier.
 */
const gchar * 
gimp_unit_get_identifier (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, NULL);

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.identifier;

  if (!gimp_unit_init (unit))
    return NULL;

  return gimp_unit_defs[unit].identifier;
}

/**
 * gimp_unit_get_symbol:
 * @unit: The unit you want to know the symbol of.
 *
 * This is e.g. "''" for UNIT_INCH.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's symbol.
 */
const gchar *
gimp_unit_get_symbol (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, NULL);

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.symbol;

  if (!gimp_unit_init (unit))
    return NULL;

  return gimp_unit_defs[unit].symbol;
}

/**
 * gimp_unit_get_abbreviation:
 * @unit: The unit you want to know the abbreviation of.
 *
 * For built-in units, this function returns the translated abbreviation
 * of the unit.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's abbreviation.
 */
const gchar *
gimp_unit_get_abbreviation (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, NULL);

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.abbreviation;

  if (!gimp_unit_init (unit))
    return NULL;

  return gimp_unit_defs[unit].abbreviation;
}

/**
 * gimp_unit_get_singular:
 * @unit: The unit you want to know the singular form of.
 *
 * For built-in units, this function returns the translated singular form
 * of the unit's name.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's singular form.
 */
const gchar *
gimp_unit_get_singular (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, NULL);

  if (unit == GIMP_UNIT_PERCENT)
    return gettext (gimp_unit_percent.singular);

  if (!gimp_unit_init (unit))
    return NULL;

  return gettext (gimp_unit_defs[unit].singular);
}

/**
 * gimp_unit_get_plural:
 * @unit: The unit you want to know the plural form of.
 *
 * For built-in units, this function returns the translated plural form
 * of the unit's name.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's plural form.
 *
 */
const gchar *
gimp_unit_get_plural (GimpUnit unit)
{
  g_return_val_if_fail (unit >= GIMP_UNIT_PIXEL, NULL);

  if (unit == GIMP_UNIT_PERCENT)
    return gettext (gimp_unit_percent.plural);

  if (!gimp_unit_init (unit))
    return NULL;

  return gettext (gimp_unit_defs[unit].plural);
}
