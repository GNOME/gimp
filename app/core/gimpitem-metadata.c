/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpitem-metadata.h"


/* public functions */

void
gimp_item_set_attributes (GimpItem *item,
                          GimpAttributes *attributes,
                          gboolean        push_undo)
{
  GimpViewable *viewable;

  g_return_if_fail (GIMP_IS_ITEM (item));

  if (push_undo)
    gimp_image_undo_push_item_attributes (gimp_item_get_image (item),
                                          NULL,
                                          item);

  viewable = GIMP_VIEWABLE (item);

  if (viewable)
    {
      gimp_viewable_set_attributes (viewable, attributes);
    }
}

GimpAttributes *
gimp_item_get_attributes (GimpItem *item)
{
  GimpViewable   *viewable;
  GimpAttributes *attributes;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  viewable = GIMP_VIEWABLE (item);

  if (viewable)
    {
      attributes = gimp_viewable_get_attributes (viewable);
      return attributes;
    }

  return NULL;
}
