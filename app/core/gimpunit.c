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
} GimpUnitDef;


/*  these are the built-in units
 */
static const GimpUnitDef gimp_unit_defs[GIMP_UNIT_END] =
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
static const GimpUnitDef gimp_unit_percent =
{
    FALSE,  0.0, 0, "percent",     "%",  "%",
    NC_("singular", "percent"),    NC_("plural", "percent")
};


/* private functions */

static GimpUnitDef *
_gimp_unit_get_user_unit (Gimp     *gimp,
                          GimpUnit  unit)
{
  return g_list_nth_data (gimp->user_units, unit - GIMP_UNIT_END);
}


/* public functions */

gint
_gimp_unit_get_number_of_units (Gimp *gimp)
{
  return GIMP_UNIT_END + gimp->n_user_units;
}

gint
_gimp_unit_get_number_of_built_in_units (Gimp *gimp)
{
  return GIMP_UNIT_END;
}

GimpUnit
_gimp_unit_new (Gimp        *gimp,
                const gchar *identifier,
                gdouble      factor,
                gint         digits,
                const gchar *symbol,
                const gchar *abbreviation,
                const gchar *singular,
                const gchar *plural)
{
  GimpUnitDef *user_unit = g_slice_new0 (GimpUnitDef);

  user_unit->delete_on_exit = TRUE;
  user_unit->factor         = factor;
  user_unit->digits         = digits;
  user_unit->identifier     = g_strdup (identifier);
  user_unit->symbol         = g_strdup (symbol);
  user_unit->abbreviation   = g_strdup (abbreviation);
  user_unit->singular       = g_strdup (singular);
  user_unit->plural         = g_strdup (plural);

  gimp->user_units = g_list_append (gimp->user_units, user_unit);
  gimp->n_user_units++;

  return GIMP_UNIT_END + gimp->n_user_units - 1;
}

gboolean
_gimp_unit_get_deletion_flag (Gimp     *gimp,
                              GimpUnit  unit)
{
  g_return_val_if_fail (unit < (GIMP_UNIT_END + gimp->n_user_units), FALSE);

  if (unit < GIMP_UNIT_END)
    return FALSE;

  return _gimp_unit_get_user_unit (gimp, unit)->delete_on_exit;
}

void
_gimp_unit_set_deletion_flag (Gimp     *gimp,
                              GimpUnit  unit,
                              gboolean  deletion_flag)
{
  g_return_if_fail ((unit >= GIMP_UNIT_END) &&
                    (unit < (GIMP_UNIT_END + gimp->n_user_units)));

  _gimp_unit_get_user_unit (gimp, unit)->delete_on_exit =
    deletion_flag ? TRUE : FALSE;
}

gdouble
_gimp_unit_get_factor (Gimp     *gimp,
                       GimpUnit  unit)
{
  g_return_val_if_fail (unit < (GIMP_UNIT_END + gimp->n_user_units) ||
                        (unit == GIMP_UNIT_PERCENT),
                        gimp_unit_defs[GIMP_UNIT_INCH].factor);

  if (unit < GIMP_UNIT_END)
    return gimp_unit_defs[unit].factor;

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.factor;

  return _gimp_unit_get_user_unit (gimp, unit)->factor;
}

gint
_gimp_unit_get_digits (Gimp     *gimp,
                       GimpUnit  unit)
{
  g_return_val_if_fail (unit < (GIMP_UNIT_END + gimp->n_user_units) ||
                        (unit == GIMP_UNIT_PERCENT),
                        gimp_unit_defs[GIMP_UNIT_INCH].digits);

  if (unit < GIMP_UNIT_END)
    return gimp_unit_defs[unit].digits;

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.digits;

  return _gimp_unit_get_user_unit (gimp, unit)->digits;
}

const gchar *
_gimp_unit_get_identifier (Gimp     *gimp,
                           GimpUnit  unit)
{
  g_return_val_if_fail ((unit < (GIMP_UNIT_END + gimp->n_user_units)) ||
                        (unit == GIMP_UNIT_PERCENT),
                        gimp_unit_defs[GIMP_UNIT_INCH].identifier);

  if (unit < GIMP_UNIT_END)
    return gimp_unit_defs[unit].identifier;

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.identifier;

  return _gimp_unit_get_user_unit (gimp, unit)->identifier;
}

const gchar *
_gimp_unit_get_symbol (Gimp     *gimp,
                       GimpUnit  unit)
{
  g_return_val_if_fail ((unit < (GIMP_UNIT_END + gimp->n_user_units)) ||
                        (unit == GIMP_UNIT_PERCENT),
                        gimp_unit_defs[GIMP_UNIT_INCH].symbol);

  if (unit < GIMP_UNIT_END)
    return gimp_unit_defs[unit].symbol;

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.symbol;

  return _gimp_unit_get_user_unit (gimp, unit)->symbol;
}

const gchar *
_gimp_unit_get_abbreviation (Gimp     *gimp,
                             GimpUnit  unit)
{
  g_return_val_if_fail ((unit < (GIMP_UNIT_END + gimp->n_user_units)) ||
                        (unit == GIMP_UNIT_PERCENT),
                        gimp_unit_defs[GIMP_UNIT_INCH].abbreviation);

  if (unit < GIMP_UNIT_END)
    return gimp_unit_defs[unit].abbreviation;

  if (unit == GIMP_UNIT_PERCENT)
    return gimp_unit_percent.abbreviation;

  return _gimp_unit_get_user_unit (gimp, unit)->abbreviation;
}

const gchar *
_gimp_unit_get_singular (Gimp     *gimp,
                         GimpUnit  unit)
{
  g_return_val_if_fail ((unit < (GIMP_UNIT_END + gimp->n_user_units)) ||
                        (unit == GIMP_UNIT_PERCENT),
                        gimp_unit_defs[GIMP_UNIT_INCH].singular);

  if (unit < GIMP_UNIT_END)
    return g_dpgettext2 (NULL, "unit-singular", gimp_unit_defs[unit].singular);

  if (unit == GIMP_UNIT_PERCENT)
    return g_dpgettext2 (NULL, "unit-singular", gimp_unit_percent.singular);

  return _gimp_unit_get_user_unit (gimp, unit)->singular;
}

const gchar *
_gimp_unit_get_plural (Gimp     *gimp,
                       GimpUnit  unit)
{
  g_return_val_if_fail ((unit < (GIMP_UNIT_END + gimp->n_user_units)) ||
                        (unit == GIMP_UNIT_PERCENT),
                        gimp_unit_defs[GIMP_UNIT_INCH].plural);

  if (unit < GIMP_UNIT_END)
    return g_dpgettext2 (NULL, "unit-plural", gimp_unit_defs[unit].plural);

  if (unit == GIMP_UNIT_PERCENT)
    return g_dpgettext2 (NULL, "unit-plural", gimp_unit_percent.plural);

  return _gimp_unit_get_user_unit (gimp, unit)->plural;
}


/* The sole purpose of this function is to release the allocated
 * memory. It must only be used from gimp_units_exit().
 */
void
gimp_user_units_free (Gimp *gimp)
{
  GList *list;

  for (list = gimp->user_units; list; list = g_list_next (list))
    {
      GimpUnitDef *user_unit = list->data;

      g_free (user_unit->identifier);
      g_free (user_unit->symbol);
      g_free (user_unit->abbreviation);
      g_free (user_unit->singular);
      g_free (user_unit->plural);

      g_slice_free (GimpUnitDef, user_unit);
    }

  g_list_free (gimp->user_units);
  gimp->user_units   = NULL;
  gimp->n_user_units = 0;
}
