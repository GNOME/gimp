/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpunit.h
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* NOTE:
 *
 * This file serves as header for both app/gimpunit.c and libgimp/gimpunit.c
 * because the unit functions are needed by widgets which are used by both
 * the gimp app and plugins.
 */

#ifndef __GIMPUNIT_H__
#define __GIMPUNIT_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
  UNIT_PIXEL = 0,
  UNIT_INCH  = 1,
  UNIT_MM    = 2,
  UNIT_POINT = 3,
  UNIT_PICA  = 4,
  UNIT_END   = 5,      /*  never use UNIT_END but
			*  gimp_unit_get_number_of_units() instead
			*/

  UNIT_PERCENT = 65536 /*  this one does not really belong here but it's
			*  convenient to use the unit system for the
			*  various strings (symbol, singular, ...)
			*
			*  you can only ask it for it's strings, asking for
			*  factor, digits or deletion_flag will produce
			*  an error.
			*/
} GUnit;


gint     gimp_unit_get_number_of_units          (void);
gint     gimp_unit_get_number_of_built_in_units (void);

/* Create a new user unit and returns it's ID.
 *
 * Note that a new unit is always created with it's deletion flag
 * set to TRUE. You will have to set it to FALSE after creation to make
 * the unit definition persistant.
 */
GUnit    gimp_unit_new                 (gchar   *identifier,
					gdouble  factor,
					gint     digits,
					gchar   *symbol,
					gchar   *abbreviation,
					gchar   *singular,
					gchar   *plural);

/* The following functions fall back to inch (not pixel, as pixel is not
 * a 'real' unit) if the value passed is out of range.
 *
 * Trying to change the deletion flag of built-in units will be ignored.
 */

/* If the deletion flag for a unit is TRUE on GIMP exit, this unit
 * will not be saved in the user units database.
 */
guint    gimp_unit_get_deletion_flag   (GUnit  unit);
void     gimp_unit_set_deletion_flag   (GUnit  unit,
					guint  deletion_flag);

/* The meaning of 'factor' is:
 * distance_in_units == ( factor * distance_in_inches )
 *
 * Returns 0 for unit == UNIT_PIXEL as we don't have resolution info here
 */
gdouble  gimp_unit_get_factor          (GUnit  unit);

/* The following function gives a hint how many digits a spinbutton
 * should provide to get approximately the accuracy of an inch-spinbutton
 * with two digits.
 *
 * Returns 0 for unit == UNIT_PIXEL as we don't have resolution info here.
 */
gint     gimp_unit_get_digits          (GUnit  unit);

/* NOTE:
 *
 * the gchar pointer returned is constant in the gimp application but must
 * be g_free()'d by plug-ins.
 */

/* This one is an untranslated string for gimprc */
gchar *  gimp_unit_get_identifier      (GUnit  unit);

gchar *  gimp_unit_get_symbol          (GUnit  unit);
gchar *  gimp_unit_get_abbreviation    (GUnit  unit);
gchar *  gimp_unit_get_singular        (GUnit  unit);
gchar *  gimp_unit_get_plural          (GUnit  unit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /*  __GIMPUNIT_H__  */
