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
#include "core/gimppalette.h"

#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimpcolormapselection.h"

#include "actions.h"
#include "colormap-commands.h"


/*  public functions  */

void
colormap_edit_color_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (data);

  gimp_colormap_editor_edit_color (editor);
}

void
colormap_delete_color_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (data);

  gimp_colormap_editor_delete_color (editor);
}

void
colormap_add_color_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpContext *context;
  GimpImage   *image;
  gboolean     background;
  return_if_no_context (context, data);
  return_if_no_image (image, data);

  background = (gboolean) g_variant_get_int32 (value);

  if (gimp_palette_get_n_colors (gimp_image_get_colormap_palette ((image))) < 256)
    {
      GeglColor *color;

      if (background)
        color = gimp_context_get_background (context);
      else
        color = gimp_context_get_foreground (context);

      gimp_image_add_colormap_entry (image, color);
      gimp_image_flush (image);
    }
}

void
colormap_to_selection_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpColormapSelection *selection;
  GimpColormapEditor    *editor;
  GimpImage             *image;
  GList                 *drawables;
  GimpChannelOps         op;
  gint                   col_index;

  return_if_no_image (image, data);

  editor    = GIMP_COLORMAP_EDITOR (data);
  selection = GIMP_COLORMAP_SELECTION (editor->selection);
  col_index = gimp_colormap_selection_get_index (selection, NULL);

  op = (GimpChannelOps) g_variant_get_int32 (value);

  drawables = gimp_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      /* We should not reach this anyway as colormap-actions.c normally takes
       * care at making the action insensitive when the item selection is wrong.
       */
      g_warning ("This action requires exactly one selected drawable.");
      g_list_free (drawables);
      return;
    }

  gimp_channel_select_by_index (gimp_image_get_mask (image),
                                drawables->data,
                                col_index, op,
                                FALSE, 0.0, 0.0);

  g_list_free (drawables);
  gimp_image_flush (image);
}
