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

#include "gimpcolortypes.h"

#include "gimphsv.h"


/*  HSV functions  */

void
gimp_hsv_set (GimpHSV *hsv,
              gdouble  h,
              gdouble  s,
              gdouble  v)
{
  g_return_if_fail (hsv != NULL);

  hsv->h = h;
  hsv->s = s;
  hsv->v = v;
}

void
gimp_hsv_clamp (GimpHSV *hsv)
{
  g_return_if_fail (hsv != NULL);

  hsv->h -= (gint) hsv->h;

  if (hsv->h < 0)
    hsv->h += 1.0;

  hsv->s = CLAMP (hsv->s, 0.0, 1.0);
  hsv->v = CLAMP (hsv->v, 0.0, 1.0);
  hsv->a = CLAMP (hsv->a, 0.0, 1.0);
}

void
gimp_hsva_set (GimpHSV *hsva,
               gdouble  h,
               gdouble  s,
               gdouble  v,
               gdouble  a)
{
  g_return_if_fail (hsva != NULL);

  hsva->h = h;
  hsva->s = s;
  hsva->v = v;
  hsva->a = a;
}
