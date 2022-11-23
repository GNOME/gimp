/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaunitcache.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "ligmaunitcache.h"
#include "ligmaunit_pdb.h"

#include "libligma-intl.h"

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
} LigmaUnitDef;


static LigmaUnitDef * ligma_unit_defs         = NULL;
static LigmaUnit      ligma_units_initialized = 0;

/*  not a unit at all but kept here to have the strings in one place
 */
static const LigmaUnitDef ligma_unit_percent =
{
  0.0, 0, "percent", "%", "%",  N_("percent"), N_("percent")
};


static void  ligma_unit_def_init (LigmaUnitDef *unit_def,
                                 LigmaUnit     unit);


static gboolean
ligma_unit_init (LigmaUnit unit)
{
  gint i, n;

  if (unit < ligma_units_initialized)
    return TRUE;

  n = _ligma_unit_get_number_of_units ();

  if (unit >= n)
    return FALSE;

  ligma_unit_defs = g_renew (LigmaUnitDef, ligma_unit_defs, n);

  for (i = ligma_units_initialized; i < n; i++)
    {
      ligma_unit_def_init (&ligma_unit_defs[i], i);
    }

  ligma_units_initialized = n;

  return TRUE;
}

static void
ligma_unit_def_init (LigmaUnitDef *unit_def,
                    LigmaUnit     unit)
{
  unit_def->factor       = _ligma_unit_get_factor (unit);
  unit_def->digits       = _ligma_unit_get_digits (unit);
  unit_def->identifier   = _ligma_unit_get_identifier (unit);
  unit_def->symbol       = _ligma_unit_get_symbol (unit);
  unit_def->abbreviation = _ligma_unit_get_abbreviation (unit);
  unit_def->singular     = _ligma_unit_get_singular (unit);
  unit_def->plural       = _ligma_unit_get_plural (unit);
}

gint
_ligma_unit_cache_get_number_of_units (void)
{
  return _ligma_unit_get_number_of_units ();
}

gint
_ligma_unit_cache_get_number_of_built_in_units (void)
{
  return LIGMA_UNIT_END;
}

LigmaUnit
_ligma_unit_cache_new (gchar   *identifier,
                      gdouble  factor,
                      gint     digits,
                      gchar   *symbol,
                      gchar   *abbreviation,
                      gchar   *singular,
                      gchar   *plural)
{
  return _ligma_unit_new (identifier,
                         factor,
                         digits,
                         symbol,
                         abbreviation,
                         singular,
                         plural);
}

gboolean
_ligma_unit_cache_get_deletion_flag (LigmaUnit unit)
{
  if (unit < LIGMA_UNIT_END)
    return FALSE;

  return _ligma_unit_get_deletion_flag (unit);
}

void
_ligma_unit_cache_set_deletion_flag (LigmaUnit unit,
                                    gboolean deletion_flag)
{
  if (unit < LIGMA_UNIT_END)
    return;

  _ligma_unit_set_deletion_flag (unit,
                                deletion_flag);
}

gdouble
_ligma_unit_cache_get_factor (LigmaUnit unit)
{
  g_return_val_if_fail (unit >= LIGMA_UNIT_INCH, 1.0);

  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.factor;

  if (!ligma_unit_init (unit))
    return 1.0;

  return ligma_unit_defs[unit].factor;
}

gint
_ligma_unit_cache_get_digits (LigmaUnit unit)
{
  g_return_val_if_fail (unit >= LIGMA_UNIT_INCH, 0);

  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.digits;

  if (!ligma_unit_init (unit))
    return 0;

  return ligma_unit_defs[unit].digits;
}

const gchar *
_ligma_unit_cache_get_identifier (LigmaUnit unit)
{
  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.identifier;

  if (!ligma_unit_init (unit))
    return NULL;

  return ligma_unit_defs[unit].identifier;
}

const gchar *
_ligma_unit_cache_get_symbol (LigmaUnit unit)
{
  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.symbol;

  if (!ligma_unit_init (unit))
    return NULL;

  return ligma_unit_defs[unit].symbol;
}

const gchar *
_ligma_unit_cache_get_abbreviation (LigmaUnit unit)
{
  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.abbreviation;

  if (!ligma_unit_init (unit))
    return NULL;

  return ligma_unit_defs[unit].abbreviation;
}

const gchar *
_ligma_unit_cache_get_singular (LigmaUnit unit)
{
  if (unit == LIGMA_UNIT_PERCENT)
    return gettext (ligma_unit_percent.singular);

  if (!ligma_unit_init (unit))
    return NULL;

  return gettext (ligma_unit_defs[unit].singular);
}

const gchar *
_ligma_unit_cache_get_plural (LigmaUnit unit)
{
  if (unit == LIGMA_UNIT_PERCENT)
    return gettext (ligma_unit_percent.plural);

  if (!ligma_unit_init (unit))
    return NULL;

  return gettext (ligma_unit_defs[unit].plural);
}
