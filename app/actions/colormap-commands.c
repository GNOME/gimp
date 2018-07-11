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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "actions-types.h"

#include "core/gimpchannel-select.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"

#include "widgets/gimpcolormapeditor.h"

#include "actions.h"
#include "colormap-commands.h"


/*  public functions  */

void
colormap_edit_color_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (data);

  gimp_colormap_editor_edit_color (editor);
}

void
colormap_add_color_cmd_callback (GtkAction *action,
                                 gint       value,
                                 gpointer   data)
{
  GimpContext *context;
  GimpImage   *image;
  return_if_no_context (context, data);
  return_if_no_image (image, data);

  if (gimp_image_get_colormap_size (image) < 256)
    {
      GimpRGB color;

      if (value)
        gimp_context_get_background (context, &color);
      else
        gimp_context_get_foreground (context, &color);

      gimp_image_add_colormap_entry (image, &color);
      gimp_image_flush (image);
    }
}

void
colormap_to_selection_cmd_callback (GtkAction *action,
                                    gint       value,
                                    gpointer   data)
{
  GimpColormapEditor *editor;
  GimpImage          *image;
  GimpChannelOps      op;
  return_if_no_image (image, data);

  editor = GIMP_COLORMAP_EDITOR (data);

  op = (GimpChannelOps) value;

  gimp_channel_select_by_index (gimp_image_get_mask (image),
                                gimp_image_get_active_drawable (image),
                                editor->col_index,
                                op,
                                FALSE, 0.0, 0.0);

  gimp_image_flush (image);
}
