/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include <glib-object.h>

#include "gimpcolortypes.h"

#include "gimphsl.h"


/*
 * GIMP_TYPE_HSL
 */

static GimpHSL * gimp_hsl_copy (const GimpHSL *hsl);


GType
gimp_hsl_get_type (void)
{
  static GType hsl_type = 0;

  if (!hsl_type)
    hsl_type = g_boxed_type_register_static ("GimpHSL",
                                              (GBoxedCopyFunc) gimp_hsl_copy,
                                              (GBoxedFreeFunc) g_free);

  return hsl_type;
}

static GimpHSL *
gimp_hsl_copy (const GimpHSL *hsl)
{
  return g_memdup (hsl, sizeof (GimpHSL));
}


/*  HSL functions  */

/**
 * gimp_hsl_set:
 * @hsl:
 * @h:
 * @s:
 * @l:
 *
 * Since: 2.8
 **/
void
gimp_hsl_set (GimpHSL *hsl,
              gdouble  h,
              gdouble  s,
              gdouble  l)
{
  g_return_if_fail (hsl != NULL);

  hsl->h = h;
  hsl->s = s;
  hsl->l = l;
}

/**
 * gimp_hsl_set_alpha:
 * @hsl:
 * @a:
 *
 * Since: 2.10
 **/
void
gimp_hsl_set_alpha (GimpHSL *hsl,
                    gdouble a)
{
  g_return_if_fail (hsl != NULL);

  hsl->a = a;
}
