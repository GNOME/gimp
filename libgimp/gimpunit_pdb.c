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
  float    factor;
  gint     digits;
  gchar   *symbol;
  gchar   *abbreviation;
  gchar   *singular;
  gchar   *plural;
} GimpUnitDef;

static GimpUnitDef gimp_unit_defs[UNIT_END] =
{
  /* 'pseudo' unit */
  {  0.0,      0, "px", "px", N_("pixel"),      N_("pixels") },

  /* 'standard' units */
  {  1.0,      2, "''", "in", N_("inch"),       N_("inches") },
  {  2.54,     2, "cm", "cm", N_("centimeter"), N_("centimeters") },

  /* 'professional' units */
  { 72.0,      0, "pt", "pt", N_("point"),      N_("points") },
  {  6.0,      1, "pc", "pc", N_("pica"),       N_("picas") },

  /* convenience units */
  { 25.4,      1, "mm", "mm", N_("millimeter"), N_("millimeters") },
  {  0.0254,   4, "m",  "m",  N_("meter"),      N_("meters") },
  {  1.0/12.0, 4, "'",  "ft", N_("foot"),       N_("feet") },
  {  1.0/36.0, 4, "yd", "yd", N_("yard"),       N_("yards") }
};


/* public functions */

gfloat
gimp_unit_get_factor (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && (unit < UNIT_END),
			 gimp_unit_defs[UNIT_INCH].factor );

  return gimp_unit_defs[unit].factor;
}


gint
gimp_unit_get_digits (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && (unit < UNIT_END),
			 gimp_unit_defs[UNIT_INCH].digits );

  return gimp_unit_defs[unit].digits;
}


const gchar *
gimp_unit_get_symbol (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && (unit < UNIT_END),
			 gimp_unit_defs[UNIT_INCH].symbol );

  return gimp_unit_defs[unit].symbol;
}


const gchar *
gimp_unit_get_abbreviation (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && (unit < UNIT_END),
			 gimp_unit_defs[UNIT_INCH].abbreviation );

  return gimp_unit_defs[unit].abbreviation;
}


const gchar *
gimp_unit_get_singular (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && (unit < UNIT_END),
			 gettext(gimp_unit_defs[UNIT_INCH].singular) );

  return gettext(gimp_unit_defs[unit].singular);
}


const gchar *
gimp_unit_get_plural (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && (unit < UNIT_END),
			 gettext(gimp_unit_defs[UNIT_INCH].plural) );

  return gettext(gimp_unit_defs[unit].plural);
}
