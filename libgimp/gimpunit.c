/* gimpunit.c
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

#include "gimpunit.h"
#include "libgimp/gimpintl.h"

/* internal structures */

typedef struct {
  guint    delete_on_exit;
  float    factor;
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
  { FALSE,  0.0,  0, "pixels",      "px", "px", N_("pixel"),      N_("pixels") },

  /* standard units */
  { FALSE,  1.0,  2, "inches",      "''", "in", N_("inch"),       N_("inches") },
  { FALSE,  2.54, 2, "centimeters", "cm", "cm", N_("centimeter"), N_("centimeters") },

  /* professional units */
  { FALSE, 72.0,  0, "points",      "pt", "pt", N_("point"),      N_("points") },
  { FALSE,  6.0,  1, "picas",       "pc", "pc", N_("pica"),       N_("picas") },
};

static GSList* user_units = NULL;
static gint    number_of_user_units = 0;


/* private functions */

GimpUnitDef *
gimp_unit_get_user_unit (GUnit unit)
{
  return g_slist_nth_data (user_units, unit - UNIT_END + 1);
}


/* public functions */

gint
gimp_unit_get_number_of_units (void)
{
  return UNIT_END + number_of_user_units;
}

gint
gimp_unit_get_number_of_built_in_units (void)
{
  return UNIT_END;
}


GUnit
gimp_unit_new (void)
{
  GimpUnitDef *user_unit;

  user_unit = g_malloc (sizeof (GimpUnitDef));
  user_unit->delete_on_exit = TRUE;
  user_unit->factor = 1.0;
  user_unit->digits = 2;
  user_unit->identifier = NULL;
  user_unit->symbol = NULL;
  user_unit->abbreviation = NULL;
  user_unit->singular = NULL;
  user_unit->plural = NULL;

  if (user_units == NULL)
    user_units = g_slist_alloc ();

  g_slist_append (user_units, user_unit);
  number_of_user_units++;

  return UNIT_END + number_of_user_units - 1;
}


guint
gimp_unit_get_deletion_flag (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && 
			 (unit < (UNIT_END + number_of_user_units)), FALSE);

  if (unit < UNIT_END)
    return FALSE;

  return gimp_unit_get_user_unit (unit)->delete_on_exit;
}

void
gimp_unit_set_deletion_flag (GUnit  unit,
			     guint  deletion_flag)
{
  g_return_if_fail ( (unit >= UNIT_END) && 
		     (unit < (UNIT_END + number_of_user_units)));

  gimp_unit_get_user_unit (unit)->delete_on_exit = deletion_flag;
}


gfloat
gimp_unit_get_factor (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && 
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].factor );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].factor;

  return gimp_unit_get_user_unit (unit)->factor;
}

void
gimp_unit_set_factor (GUnit  unit,
		      gfloat factor)
{
  g_return_if_fail ( (unit >= UNIT_END) && 
		     (unit < (UNIT_END + number_of_user_units)));

  gimp_unit_get_user_unit (unit)->factor = factor;
}


gint
gimp_unit_get_digits (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].digits );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].digits;

  return gimp_unit_get_user_unit (unit)->digits;
}

void
gimp_unit_set_digits (GUnit  unit,
		      gint   digits)
{
  g_return_if_fail ( (unit >= UNIT_END) && 
		     (unit < (UNIT_END + number_of_user_units)));

  gimp_unit_get_user_unit (unit)->digits = digits;
}


const gchar * 
gimp_unit_get_identifier (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && 
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].identifier );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].identifier;

  return gimp_unit_get_user_unit (unit)->identifier;
}

void
gimp_unit_set_identifier (GUnit  unit,
			  gchar *identifier)
{
  GimpUnitDef *user_unit;

  g_return_if_fail ( (unit >= UNIT_END) && 
		     (unit < (UNIT_END + number_of_user_units)));

  user_unit = gimp_unit_get_user_unit (unit);
  if (user_unit->identifier)
    g_free (user_unit->identifier);
  user_unit->identifier = g_strdup (identifier);
}


const gchar *
gimp_unit_get_symbol (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].symbol );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].symbol;

  return gimp_unit_get_user_unit (unit)->symbol;
}

void
gimp_unit_set_symbol (GUnit  unit,
		      gchar *symbol)
{
  GimpUnitDef *user_unit;

  g_return_if_fail ( (unit >= UNIT_END) && 
		     (unit < (UNIT_END + number_of_user_units)));

  user_unit = gimp_unit_get_user_unit (unit);
  if (user_unit->symbol)
    g_free (user_unit->symbol);
  user_unit->symbol = g_strdup (symbol);
}


const gchar *
gimp_unit_get_abbreviation (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].abbreviation );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].abbreviation;

  return gimp_unit_get_user_unit (unit)->abbreviation;
}


const gchar *
gimp_unit_get_singular (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gettext(gimp_unit_defs[UNIT_INCH].singular) );

  if (unit < UNIT_END)
    return gettext(gimp_unit_defs[unit].singular);

  return gimp_unit_get_user_unit (unit)->singular;
}

void
gimp_unit_set_singular (GUnit  unit,
			gchar *singular)
{
  GimpUnitDef *user_unit;

  g_return_if_fail ( (unit >= UNIT_END) && 
		     (unit < (UNIT_END + number_of_user_units)));

  user_unit = gimp_unit_get_user_unit (unit);
  if (user_unit->singular)
    g_free (user_unit->singular);
  user_unit->symbol = g_strdup (singular);
}


const gchar *
gimp_unit_get_plural (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gettext(gimp_unit_defs[UNIT_INCH].plural) );

  if (unit < UNIT_END)
    return gettext(gimp_unit_defs[unit].plural);

  return gimp_unit_get_user_unit (unit)->plural;
}

void
gimp_unit_set_plural (GUnit  unit,
		      gchar *plural)
{
  GimpUnitDef *user_unit;

  g_return_if_fail ( (unit >= UNIT_END) && 
		     (unit < (UNIT_END + number_of_user_units)));

  user_unit = gimp_unit_get_user_unit (unit);
  if (user_unit->plural)
    g_free (user_unit->plural);
  user_unit->symbol = g_strdup (plural);
}
