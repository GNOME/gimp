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


G_DEFINE_TYPE (GimpSelection, gimp_selection, GIMP_TYPE_CHANNEL)

#define parent_class gimp_selection_parent_class

static void
gimp_selection_class_init (GimpSelectionClass *klass)
{
}

static void
gimp_selection_init (GimpSelection *selection)
{
}

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
  GimpLayer *selection;
  gint32     selection_id = -1;

  selection = gimp_selection_float (gimp_image_get_by_id (image_ID),
                                    GIMP_DRAWABLE (gimp_item_get_by_id (drawable_ID)),
                                    offx,
                                    offy);
  if (selection)
    selection_id = gimp_item_get_id (GIMP_ITEM (selection));

  return selection_id;
}
