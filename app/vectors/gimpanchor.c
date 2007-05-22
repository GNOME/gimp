/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpanchor.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "glib-object.h"

#include "vectors-types.h"

#include "gimpanchor.h"


GType
gimp_anchor_get_type (void)
{
  static GType anchor_type = 0;

  if (!anchor_type)
    anchor_type = g_boxed_type_register_static ("GimpAnchor",
                                                (GBoxedCopyFunc) gimp_anchor_copy,
                                                (GBoxedFreeFunc) gimp_anchor_free);

  return anchor_type;
}

GimpAnchor *
gimp_anchor_new (GimpAnchorType    type,
                 const GimpCoords *position)
{
  GimpAnchor *anchor = g_slice_new0 (GimpAnchor);

  anchor->type = type;

  if (position)
    anchor->position = *position;

  return anchor;
}

GimpAnchor *
gimp_anchor_copy (const GimpAnchor *anchor)
{
  GimpAnchor *new;

  g_return_val_if_fail (anchor != NULL, NULL);

  new = g_slice_new (GimpAnchor);

  *new = *anchor;

  return new;
}

void
gimp_anchor_free (GimpAnchor *anchor)
{
  g_return_if_fail (anchor != NULL);

  g_slice_free (GimpAnchor, anchor);
}
