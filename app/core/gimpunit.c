/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmaunit.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmaunit.h"

#include "ligma-intl.h"


/* internal structures */

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
} LigmaUnitDef;


/*  these are the built-in units
 */
static const LigmaUnitDef ligma_unit_defs[LIGMA_UNIT_END] =
{
  /* pseudo unit */
  { FALSE,  0.0, 0, "pixels",      "px", "px",
    NC_("unit-singular", "pixel"),      NC_("unit-plural", "pixels")      },

  /* standard units */
  { FALSE,  1.0, 2, "inches",      "''", "in",
    NC_("unit-singular", "inch"),       NC_("unit-plural", "inches")      },

  { FALSE, 25.4, 1, "millimeters", "mm", "mm",
    NC_("unit-singular", "millimeter"), NC_("unit-plural", "millimeters") },

  /* professional units */
  { FALSE, 72.0, 0, "points",      "pt", "pt",
    NC_("unit-singular", "point"),      NC_("unit-plural", "points")      },

  { FALSE,  6.0, 1, "picas",       "pc", "pc",
    NC_("unit-singular", "pica"),       NC_("unit-plural", "picas")       }
};

/*  not a unit at all but kept here to have the strings in one place
 */
static const LigmaUnitDef ligma_unit_percent =
{
    FALSE,  0.0, 0, "percent",     "%",  "%",
    NC_("singular", "percent"),    NC_("plural", "percent")
};


/* private functions */

static LigmaUnitDef *
_ligma_unit_get_user_unit (Ligma     *ligma,
                          LigmaUnit  unit)
{
  return g_list_nth_data (ligma->user_units, unit - LIGMA_UNIT_END);
}


/* public functions */

gint
_ligma_unit_get_number_of_units (Ligma *ligma)
{
  return LIGMA_UNIT_END + ligma->n_user_units;
}

gint
_ligma_unit_get_number_of_built_in_units (Ligma *ligma)
{
  return LIGMA_UNIT_END;
}

LigmaUnit
_ligma_unit_new (Ligma        *ligma,
                const gchar *identifier,
                gdouble      factor,
                gint         digits,
                const gchar *symbol,
                const gchar *abbreviation,
                const gchar *singular,
                const gchar *plural)
{
  LigmaUnitDef *user_unit = g_slice_new0 (LigmaUnitDef);

  user_unit->delete_on_exit = TRUE;
  user_unit->factor         = factor;
  user_unit->digits         = digits;
  user_unit->identifier     = g_strdup (identifier);
  user_unit->symbol         = g_strdup (symbol);
  user_unit->abbreviation   = g_strdup (abbreviation);
  user_unit->singular       = g_strdup (singular);
  user_unit->plural         = g_strdup (plural);

  ligma->user_units = g_list_append (ligma->user_units, user_unit);
  ligma->n_user_units++;

  return LIGMA_UNIT_END + ligma->n_user_units - 1;
}

gboolean
_ligma_unit_get_deletion_flag (Ligma     *ligma,
                              LigmaUnit  unit)
{
  g_return_val_if_fail (unit < (LIGMA_UNIT_END + ligma->n_user_units), FALSE);

  if (unit < LIGMA_UNIT_END)
    return FALSE;

  return _ligma_unit_get_user_unit (ligma, unit)->delete_on_exit;
}

void
_ligma_unit_set_deletion_flag (Ligma     *ligma,
                              LigmaUnit  unit,
                              gboolean  deletion_flag)
{
  g_return_if_fail ((unit >= LIGMA_UNIT_END) &&
                    (unit < (LIGMA_UNIT_END + ligma->n_user_units)));

  _ligma_unit_get_user_unit (ligma, unit)->delete_on_exit =
    deletion_flag ? TRUE : FALSE;
}

gdouble
_ligma_unit_get_factor (Ligma     *ligma,
                       LigmaUnit  unit)
{
  g_return_val_if_fail (unit < (LIGMA_UNIT_END + ligma->n_user_units) ||
                        (unit == LIGMA_UNIT_PERCENT),
                        ligma_unit_defs[LIGMA_UNIT_INCH].factor);

  if (unit < LIGMA_UNIT_END)
    return ligma_unit_defs[unit].factor;

  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.factor;

  return _ligma_unit_get_user_unit (ligma, unit)->factor;
}

gint
_ligma_unit_get_digits (Ligma     *ligma,
                       LigmaUnit  unit)
{
  g_return_val_if_fail (unit < (LIGMA_UNIT_END + ligma->n_user_units) ||
                        (unit == LIGMA_UNIT_PERCENT),
                        ligma_unit_defs[LIGMA_UNIT_INCH].digits);

  if (unit < LIGMA_UNIT_END)
    return ligma_unit_defs[unit].digits;

  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.digits;

  return _ligma_unit_get_user_unit (ligma, unit)->digits;
}

const gchar *
_ligma_unit_get_identifier (Ligma     *ligma,
                           LigmaUnit  unit)
{
  g_return_val_if_fail ((unit < (LIGMA_UNIT_END + ligma->n_user_units)) ||
                        (unit == LIGMA_UNIT_PERCENT),
                        ligma_unit_defs[LIGMA_UNIT_INCH].identifier);

  if (unit < LIGMA_UNIT_END)
    return ligma_unit_defs[unit].identifier;

  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.identifier;

  return _ligma_unit_get_user_unit (ligma, unit)->identifier;
}

const gchar *
_ligma_unit_get_symbol (Ligma     *ligma,
                       LigmaUnit  unit)
{
  g_return_val_if_fail ((unit < (LIGMA_UNIT_END + ligma->n_user_units)) ||
                        (unit == LIGMA_UNIT_PERCENT),
                        ligma_unit_defs[LIGMA_UNIT_INCH].symbol);

  if (unit < LIGMA_UNIT_END)
    return ligma_unit_defs[unit].symbol;

  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.symbol;

  return _ligma_unit_get_user_unit (ligma, unit)->symbol;
}

const gchar *
_ligma_unit_get_abbreviation (Ligma     *ligma,
                             LigmaUnit  unit)
{
  g_return_val_if_fail ((unit < (LIGMA_UNIT_END + ligma->n_user_units)) ||
                        (unit == LIGMA_UNIT_PERCENT),
                        ligma_unit_defs[LIGMA_UNIT_INCH].abbreviation);

  if (unit < LIGMA_UNIT_END)
    return ligma_unit_defs[unit].abbreviation;

  if (unit == LIGMA_UNIT_PERCENT)
    return ligma_unit_percent.abbreviation;

  return _ligma_unit_get_user_unit (ligma, unit)->abbreviation;
}

const gchar *
_ligma_unit_get_singular (Ligma     *ligma,
                         LigmaUnit  unit)
{
  g_return_val_if_fail ((unit < (LIGMA_UNIT_END + ligma->n_user_units)) ||
                        (unit == LIGMA_UNIT_PERCENT),
                        ligma_unit_defs[LIGMA_UNIT_INCH].singular);

  if (unit < LIGMA_UNIT_END)
    return g_dpgettext2 (NULL, "unit-singular", ligma_unit_defs[unit].singular);

  if (unit == LIGMA_UNIT_PERCENT)
    return g_dpgettext2 (NULL, "unit-singular", ligma_unit_percent.singular);

  return _ligma_unit_get_user_unit (ligma, unit)->singular;
}

const gchar *
_ligma_unit_get_plural (Ligma     *ligma,
                       LigmaUnit  unit)
{
  g_return_val_if_fail ((unit < (LIGMA_UNIT_END + ligma->n_user_units)) ||
                        (unit == LIGMA_UNIT_PERCENT),
                        ligma_unit_defs[LIGMA_UNIT_INCH].plural);

  if (unit < LIGMA_UNIT_END)
    return g_dpgettext2 (NULL, "unit-plural", ligma_unit_defs[unit].plural);

  if (unit == LIGMA_UNIT_PERCENT)
    return g_dpgettext2 (NULL, "unit-plural", ligma_unit_percent.plural);

  return _ligma_unit_get_user_unit (ligma, unit)->plural;
}


/* The sole purpose of this function is to release the allocated
 * memory. It must only be used from ligma_units_exit().
 */
void
ligma_user_units_free (Ligma *ligma)
{
  GList *list;

  for (list = ligma->user_units; list; list = g_list_next (list))
    {
      LigmaUnitDef *user_unit = list->data;

      g_free (user_unit->identifier);
      g_free (user_unit->symbol);
      g_free (user_unit->abbreviation);
      g_free (user_unit->singular);
      g_free (user_unit->plural);

      g_slice_free (LigmaUnitDef, user_unit);
    }

  g_list_free (ligma->user_units);
  ligma->user_units   = NULL;
  ligma->n_user_units = 0;
}
