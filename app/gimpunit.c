/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpunit.c
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

#include "config.h"
#include <stdlib.h>
#include <stdio.h>

/* NOTE:
 *
 * one of our header files is in libgimp/ (see the note there)
 */
#include "libgimp/gimpunit.h"

#include "unitrc.h"

#include "app_procs.h"
#include "gimprc.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

/* internal structures */

typedef struct {
  guint    delete_on_exit;
  gdouble  factor;
  gint     digits;
  gchar   *identifier;
  gchar   *symbol;
  gchar   *abbreviation;
  gchar   *singular;
  gchar   *plural;
} GimpUnitDef;

/*  these are the built-in units
 */
static GimpUnitDef gimp_unit_defs[UNIT_END] =
{
  /* pseudo unit */
  { FALSE,  0.0, 0, "pixels",      "px", "px", N_("pixel"),      N_("pixels") },

  /* standard units */
  { FALSE,  1.0, 2, "inches",      "''", "in", N_("inch"),       N_("inches") },
  { FALSE, 25.4, 1, "millimeters", "mm", "mm", N_("millimeter"), N_("millimeters") },

  /* professional units */
  { FALSE, 72.0, 0, "points",      "pt", "pt", N_("point"),      N_("points") },
  { FALSE,  6.0, 1, "picas",       "pc", "pc", N_("pica"),       N_("picas") },
};

/*  not a unit at all but kept here to have the strings in one place
 */
static GimpUnitDef gimp_unit_percent =
{
  FALSE,    0.0, 0, "percent",     "%",  "%",  N_("percent"),    N_("percent")
};

static GSList* user_units = NULL;
static gint    number_of_user_units = 0;

/* private functions */

static GimpUnitDef *
gimp_unit_get_user_unit (GUnit unit)
{
  return g_slist_nth_data (user_units, unit - UNIT_END);
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
gimp_unit_new (gchar   *identifier,
	       gdouble  factor,
	       gint     digits,
	       gchar   *symbol,
	       gchar   *abbreviation,
	       gchar   *singular,
	       gchar   *plural)
{
  GimpUnitDef *user_unit;

  user_unit = g_malloc (sizeof (GimpUnitDef));
  user_unit->delete_on_exit = TRUE;
  user_unit->factor = factor;
  user_unit->digits = digits;
  user_unit->identifier = g_strdup (identifier);
  user_unit->symbol = g_strdup (symbol);
  user_unit->abbreviation = g_strdup (abbreviation);
  user_unit->singular = g_strdup (singular);
  user_unit->plural = g_strdup (plural);

  user_units = g_slist_append (user_units, user_unit);
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

  gimp_unit_get_user_unit (unit)->delete_on_exit =
    deletion_flag ? TRUE : FALSE;
}


gdouble
gimp_unit_get_factor (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && 
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].factor );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].factor;

  return gimp_unit_get_user_unit (unit)->factor;
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


gchar * 
gimp_unit_get_identifier (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && 
			 (unit < (UNIT_END + number_of_user_units)) ||
			 (unit == UNIT_PERCENT),
			 gimp_unit_defs[UNIT_INCH].identifier );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].identifier;

  if (unit == UNIT_PERCENT)
    return gimp_unit_percent.identifier;

  return gimp_unit_get_user_unit (unit)->identifier;
}


gchar *
gimp_unit_get_symbol (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)) ||
			 (unit == UNIT_PERCENT),
			 gimp_unit_defs[UNIT_INCH].symbol );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].symbol;

  if (unit == UNIT_PERCENT)
    return gimp_unit_percent.symbol;

  return gimp_unit_get_user_unit (unit)->symbol;
}


gchar *
gimp_unit_get_abbreviation (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)) ||
			 (unit == UNIT_PERCENT),
			 gimp_unit_defs[UNIT_INCH].abbreviation );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].abbreviation;

  if (unit == UNIT_PERCENT)
    return gimp_unit_percent.abbreviation;

  return gimp_unit_get_user_unit (unit)->abbreviation;
}


gchar *
gimp_unit_get_singular (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)) ||
			 (unit == UNIT_PERCENT),
			 gettext(gimp_unit_defs[UNIT_INCH].singular) );

  if (unit < UNIT_END)
    return gettext (gimp_unit_defs[unit].singular);

  if (unit == UNIT_PERCENT)
    return gettext (gimp_unit_percent.singular);

  return gettext (gimp_unit_get_user_unit (unit)->singular);
}


gchar *
gimp_unit_get_plural (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)) ||
			 (unit == UNIT_PERCENT),
			 gettext(gimp_unit_defs[UNIT_INCH].plural) );

  if (unit < UNIT_END)
    return gettext (gimp_unit_defs[unit].plural);

  if (unit == UNIT_PERCENT)
    return gettext (gimp_unit_percent.plural);

  return gettext (gimp_unit_get_user_unit (unit)->plural);
}


/*  unitrc functions  **********/

void parse_unitrc (void)
{
  char *filename;

  filename = gimp_personal_rc_file ("unitrc");
  app_init_update_status(NULL, filename, -1);
  parse_gimprc_file (filename);
  g_free (filename);
}


void save_unitrc (void)
{
  int i;
  char *filename;
  FILE *fp;

  filename = gimp_personal_rc_file ("unitrc");

  fp = fopen (filename, "w");
  g_free (filename);
  if (!fp)
    return;

  fprintf (fp, "# GIMP unitrc\n" 
               "# This file contains your user unit database. You can\n"
	       "# modify this list with the unit editor. You are not\n"
	       "# supposed to edit it manually, but of course you can do.\n"
	       "# This file will be entirely rewritten every time you\n"
	       "# quit the gimp.\n\n");
  
  /*  save user defined units  */
  for (i = gimp_unit_get_number_of_built_in_units();
       i < gimp_unit_get_number_of_units ();
       i++)
    if (gimp_unit_get_deletion_flag (i) == FALSE)
      {
	fprintf (fp,"(unit-info \"%s\"\n", gimp_unit_get_identifier (i));
	fprintf (fp,"   (factor %f)\n", gimp_unit_get_factor (i));
	fprintf (fp,"   (digits %d)\n", gimp_unit_get_digits (i));
	fprintf (fp,"   (symbol \"%s\")\n", gimp_unit_get_symbol (i));
	fprintf (fp,"   (abbreviation \"%s\")\n", gimp_unit_get_abbreviation (i));
	fprintf (fp,"   (singular \"%s\")\n", gimp_unit_get_singular (i));
	fprintf (fp,"   (plural \"%s\"))\n\n", gimp_unit_get_plural (i));
      }
  
  fclose (fp);
}






