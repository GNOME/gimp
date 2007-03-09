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
