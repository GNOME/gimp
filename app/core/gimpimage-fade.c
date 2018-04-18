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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawableundo.h"
#include "gimpimage.h"
#include "gimpimage-fade.h"
#include "gimpimage-undo.h"


/*  public functions  */

gboolean
gimp_image_fade (GimpImage   *image,
                 GimpContext *context)
{
  GimpDrawableUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

  if (undo && undo->applied_buffer)
    {
      GimpDrawable *drawable;
      GeglBuffer   *buffer;

      drawable = GIMP_DRAWABLE (GIMP_ITEM_UNDO (undo)->item);

      g_object_ref (undo);
      buffer = g_object_ref (undo->applied_buffer);

      gimp_image_undo (image);

      gimp_drawable_apply_buffer (drawable, buffer,
                                  GEGL_RECTANGLE (0, 0,
                                                  gegl_buffer_get_width (undo->buffer),
                                                  gegl_buffer_get_height (undo->buffer)),
                                  TRUE,
                                  gimp_object_get_name (undo),
                                  gimp_context_get_opacity (context),
                                  gimp_context_get_paint_mode (context),
                                  GIMP_LAYER_COLOR_SPACE_AUTO,
                                  GIMP_LAYER_COLOR_SPACE_AUTO,
                                  GIMP_LAYER_COMPOSITE_AUTO,
                                  NULL, undo->x, undo->y);

      g_object_unref (buffer);
      g_object_unref (undo);

      return TRUE;
    }

  return FALSE;
}
