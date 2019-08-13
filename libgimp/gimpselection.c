/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpselection.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

/**
 * gimp_selection_float:
 * @image: ignored
 * @drawable: The drawable from which to float selection.
 * @offx: x offset for translation.
 * @offy: y offset for translation.
 *
 * Float the selection from the specified drawable with initial offsets
 * as specified.
 *
 * This procedure determines the region of the specified drawable that
 * lies beneath the current selection. The region is then cut from the
 * drawable and the resulting data is made into a new layer which is
 * instantiated as a floating selection. The offsets allow initial
 * positioning of the new floating selection.
 *
 * Returns: (transfer full): The floated layer.
 */
GimpLayer *
gimp_selection_float (GimpImage    *image,
                      GimpDrawable *drawable,
                      gint          offx,
                      gint          offy)
{
  return _gimp_selection_float (drawable,
                                offx,
                                offy);
}


/* Deprecated API. */


/**
 * gimp_selection_float_deprecated: (skip)
 * @image_ID: ignored
 * @drawable_ID: The drawable from which to float selection.
 * @offx: x offset for translation.
 * @offy: y offset for translation.
 *
 * Float the selection from the specified drawable with initial offsets
 * as specified.
 *
 * This procedure determines the region of the specified drawable that
 * lies beneath the current selection. The region is then cut from the
 * drawable and the resulting data is made into a new layer which is
 * instantiated as a floating selection. The offsets allow initial
 * positioning of the new floating selection.
 *
 * Returns: The floated layer.
 */
gint32
gimp_selection_float_deprecated (gint32 image_ID,
                                 gint32 drawable_ID,
                                 gint   offx,
                                 gint   offy)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GimpLayer    *selection;
  gint32        selection_id = -1;

  image    = gimp_image_new_by_id (image_ID);
  drawable = GIMP_DRAWABLE (gimp_item_new_by_id (drawable_ID));

  selection = gimp_selection_float (image, drawable,
                                    offx,
                                    offy);
  if (selection)
    selection_id = gimp_item_get_id (GIMP_ITEM (selection));

  g_object_unref (image);
  g_object_unref (drawable);
  g_object_unref (selection);

  return selection_id;
}
