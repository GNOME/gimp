/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <cairo.h>

#include "core-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "ligmadrawable.h"
#include "ligmadrawable-private.h"
#include "ligmadrawable-shadow.h"


GeglBuffer *
ligma_drawable_get_shadow_buffer (LigmaDrawable *drawable)
{
  LigmaItem   *item;
  gint        width;
  gint        height;
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  item = LIGMA_ITEM (drawable);

  width  = ligma_item_get_width  (item);
  height = ligma_item_get_height (item);
  format = ligma_drawable_get_format (drawable);

  if (drawable->private->shadow)
    {
      if ((width  != gegl_buffer_get_width  (drawable->private->shadow)) ||
          (height != gegl_buffer_get_height (drawable->private->shadow)) ||
          (format != gegl_buffer_get_format (drawable->private->shadow)))
        {
          ligma_drawable_free_shadow_buffer (drawable);
        }
      else
        {
          return drawable->private->shadow;
        }
    }

  drawable->private->shadow = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                               width, height),
                                               format);

  return drawable->private->shadow;
}

void
ligma_drawable_free_shadow_buffer (LigmaDrawable *drawable)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));

  g_clear_object (&drawable->private->shadow);
}

void
ligma_drawable_merge_shadow_buffer (LigmaDrawable *drawable,
                                   gboolean      push_undo,
                                   const gchar  *undo_desc)
{
  gint x, y;
  gint width, height;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (GEGL_IS_BUFFER (drawable->private->shadow));

  /*  A useful optimization here is to limit the update to the
   *  extents of the selection mask, as it cannot extend beyond
   *  them.
   */
  if (ligma_item_mask_intersect (LIGMA_ITEM (drawable), &x, &y, &width, &height))
    {
      GeglBuffer *buffer = g_object_ref (drawable->private->shadow);

      ligma_drawable_apply_buffer (drawable, buffer,
                                  GEGL_RECTANGLE (x, y, width, height),
                                  push_undo, undo_desc,
                                  LIGMA_OPACITY_OPAQUE,
                                  LIGMA_LAYER_MODE_REPLACE,
                                  LIGMA_LAYER_COLOR_SPACE_AUTO,
                                  LIGMA_LAYER_COLOR_SPACE_AUTO,
                                  LIGMA_LAYER_COMPOSITE_AUTO,
                                  NULL, x, y);

      g_object_unref (buffer);
    }
}
