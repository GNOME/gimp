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


struct _GimpSelection
{
  GimpChannel parent_instance;
};


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
 * gimp_selection_get_by_id:
 * @selection_id: The selection id.
 *
 * Returns a #GimpSelection representing @selection_id. This function
 * calls gimp_item_get_by_id() and returns the item if it is selection
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpSelection for
 *          @selection_id or %NULL if @selection_id does not represent
 *          a valid selection. The object belongs to libgimp and you
 *          must not modify or unref it.
 *
 * Since: 3.0
 **/
GimpSelection *
gimp_selection_get_by_id (gint32 selection_id)
{
  GimpItem *item = gimp_item_get_by_id (selection_id);

  if (GIMP_IS_SELECTION (item))
    return (GimpSelection *) item;

  return NULL;
}

/**
 * gimp_selection_float:
 * @image:       ignored
 * @drawables:   (array zero-terminated=1): The drawables from which to
 *               float selection.
 * @offx:        x offset for translation.
 * @offy:        y offset for translation.
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
 * Returns: (transfer none): The floated layer.
 */
GimpLayer *
gimp_selection_float (GimpImage     *image,
                      GimpDrawable **drawables,
                      gint           offx,
                      gint           offy)
{
  return _gimp_selection_float ((const GimpDrawable **) drawables, offx, offy);
}
