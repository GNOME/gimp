/* gimpunit.h
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

#ifndef __GIMPUNIT_H__
#define __GIMPUNIT_H__


#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* I've put this here and not to libgimp/gimpenums.h, because if this
 * file includes libgimp/gimpenums.h there is a name clash wherever
 * someone includes libgimp/gimpunit.h and app/gimpimage.h
 * (the constants RGB, GRAY and INDEXED are defined in both
 * gimpenums.h and gimpimage.h) (is this a bug? don't know...)
 */
typedef enum
{
  UNIT_PIXEL = 0,
  UNIT_INCH  = 1,
  UNIT_CM    = 2,
  UNIT_POINT = 3,
  UNIT_PICA  = 4,
  UNIT_MM    = 5,
  UNIT_METER = 6,
  UNIT_FOOT  = 7,
  UNIT_YARD  = 8,
  UNIT_END
} GUnit;


#define        gimp_unit_get_number_of_units() UNIT_END

/* the following functions fall back to inch (not pixel, as pixel is not
 * a 'real' unit) if the value passed is out of range
 */

/* the meaning of 'factor' is:
 * distance_in_units == ( factor * distance_in_inches )
 *
 * returns 0 for unit == UNIT_PIXEL as we don't have resolution info here
 */
gfloat         gimp_unit_get_factor          (GUnit unit);

/* the following function gives a hint how many digits a spinbutton
 * should provide to get approximately the accuracy of an inch-spinbutton
 * with two digits.
 *
 * returns 0 for unit == UNIT_PIXEL as we don't have resolution info here
 */
gint           gimp_unit_get_digits          (GUnit unit);

const gchar *  gimp_unit_get_symbol          (GUnit unit);
const gchar *  gimp_unit_get_abbreviation    (GUnit unit);
const gchar *  gimp_unit_get_singular        (GUnit unit);
const gchar *  gimp_unit_get_plural          (GUnit unit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /*  __GIMPUNIT_H__  */
