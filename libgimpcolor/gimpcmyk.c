/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include "libgimpmath/gimpmath.h"

#include "gimpcolortypes.h"

#include "gimpcmyk.h"


/*  CMYK functions  */

/**
 * gimp_cmyk_set:
 * @cmyk: A GimpCMYK structure which will hold the specified cmyk value.
 * @c:    The Cyan channel of the CMYK value 
 * @m:    The Magenta channel
 * @y:    The Yellow channel
 * @k:    The blacK channel
 *
 * Very basic initialiser for the internal GimpCMYK structure. Channel 
 * values are doubles in the range 0 to 1.
 **/

void
gimp_cmyk_set (GimpCMYK *cmyk,
               gdouble   c,
               gdouble   m,
               gdouble   y,
               gdouble   k)
{
  g_return_if_fail (cmyk != NULL);

  cmyk->c = c;
  cmyk->m = m;
  cmyk->y = y;
  cmyk->k = k;
}

/**
 * gimp_cmyk_set_uchar:
 *
 * The same as gimp_cmyk_set, except that channel values are unsigned 
 * chars in the range 0 to 255.
 **/

void
gimp_cmyk_set_uchar (GimpCMYK *cmyk,
                     guchar   c,
                     guchar   m,
                     guchar   y,
                     guchar   k)
{
  g_return_if_fail (cmyk != NULL);

  cmyk->c = (gdouble) c / 255.0;
  cmyk->m = (gdouble) m / 255.0;
  cmyk->y = (gdouble) y / 255.0;
  cmyk->k = (gdouble) k / 255.0;
}

/**
 * gimp_cmyk_get_uchar:
 * @cmyk: A GimpCMYK structure which will hold the specified cmyk value.
 * @c:    The Cyan channel of the CMYK value 
 * @m:    The Magenta channel
 * @y:    The Yellow channel
 * @k:    The blacK channel
 *
 * Retrieve individual channel values from a GimpCMYK structure. Channel 
 * values are pointers to unsigned chars in the range 0 to 255.
 **/

void
gimp_cmyk_get_uchar (const GimpCMYK *cmyk,
                     guchar         *c,
                     guchar         *m,
                     guchar         *y,
                     guchar         *k)
{
  g_return_if_fail (cmyk != NULL);

  if (c) *c = ROUND (CLAMP (cmyk->c, 0.0, 1.0) * 255.0);
  if (m) *m = ROUND (CLAMP (cmyk->m, 0.0, 1.0) * 255.0);
  if (y) *y = ROUND (CLAMP (cmyk->y, 0.0, 1.0) * 255.0);
  if (k) *k = ROUND (CLAMP (cmyk->k, 0.0, 1.0) * 255.0);
}


/*  CMYKA functions  */

/**
 * gimp_cmyka_set:
 * @cmyka: A GimpCMYK structure which will hold the specified cmyka value.
 * @c:    The Cyan channel of the CMYK value 
 * @m:    The Magenta channel
 * @y:    The Yellow channel
 * @k:    The blacK channel
 * @a:    The Alpha channel
 *
 * Initialiser for the internal GimpCMYK structure. Channel values are 
 * doubles in the range 0 to 1.
 **/

void
gimp_cmyka_set (GimpCMYK *cmyka,
                gdouble   c,
                gdouble   m,
                gdouble   y,
                gdouble   k,
                gdouble   a)
{
  g_return_if_fail (cmyka != NULL);

  cmyka->c = c;
  cmyka->m = m;
  cmyka->y = y;
  cmyka->k = k;
  cmyka->a = a;
}

/**
 * gimp_cmyka_set_uchar:
 *
 * The same as gimp_cmyka_set, except that channel values are unsigned chars
 * in the range 0 to 255.
 **/

void
gimp_cmyka_set_uchar (GimpCMYK *cmyka,
                      guchar    c,
                      guchar    m,
                      guchar    y,
                      guchar    k,
                      guchar    a)
{
  g_return_if_fail (cmyka != NULL);

  cmyka->c = (gdouble) c / 255.0;
  cmyka->m = (gdouble) m / 255.0;
  cmyka->y = (gdouble) y / 255.0;
  cmyka->k = (gdouble) k / 255.0;
  cmyka->a = (gdouble) a / 255.0;
}

/**
 * gimp_cmyka_get_uchar:
 * @cmyk: A GimpCMYK structure which will hold the specified cmyka value.
 * @c:    The Cyan channel of the CMYK value 
 * @m:    The Magenta channel
 * @y:    The Yellow channel
 * @k:    The blacK channel
 * @a:    The Alpha channel
 *
 * Retrieve individual channel values from a GimpCMYK structure. Channel 
 * values are pointers to unsigned chars in the range 0 to 255.
 **/

void
gimp_cmyka_get_uchar (const GimpCMYK *cmyka,
                      guchar         *c,
                      guchar         *m,
                      guchar         *y,
                      guchar         *k,
                      guchar         *a)
{
  g_return_if_fail (cmyka != NULL);

  if (c) *c = ROUND (CLAMP (cmyka->c, 0.0, 1.0) * 255.0);
  if (m) *m = ROUND (CLAMP (cmyka->m, 0.0, 1.0) * 255.0);
  if (y) *y = ROUND (CLAMP (cmyka->y, 0.0, 1.0) * 255.0);
  if (k) *k = ROUND (CLAMP (cmyka->k, 0.0, 1.0) * 255.0);
  if (a) *a = ROUND (CLAMP (cmyka->a, 0.0, 1.0) * 255.0);
}
