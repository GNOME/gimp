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

#include <gegl.h>
#include <gtk/gtk.h>

#include "actions-types.h"

#include "core/ligmachannel-select.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-colormap.h"

#include "widgets/ligmacolormapeditor.h"
#include "widgets/ligmacolormapselection.h"

#include "actions.h"
#include "colormap-commands.h"


/*  public functions  */

void
colormap_edit_color_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaColormapEditor *editor = LIGMA_COLORMAP_EDITOR (data);

  ligma_colormap_editor_edit_color (editor);
}

void
colormap_add_color_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaContext *context;
  LigmaImage   *image;
  gboolean     background;
  return_if_no_context (context, data);
  return_if_no_image (image, data);

  background = (gboolean) g_variant_get_int32 (value);

  if (ligma_image_get_colormap_size (image) < 256)
    {
      LigmaRGB color;

      if (background)
        ligma_context_get_background (context, &color);
      else
        ligma_context_get_foreground (context, &color);

      ligma_image_add_colormap_entry (image, &color);
      ligma_image_flush (image);
    }
}

void
colormap_to_selection_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaColormapSelection *selection;
  LigmaColormapEditor    *editor;
  LigmaImage             *image;
  GList                 *drawables;
  LigmaChannelOps         op;
  gint                   col_index;

  return_if_no_image (image, data);

  editor    = LIGMA_COLORMAP_EDITOR (data);
  selection = LIGMA_COLORMAP_SELECTION (editor->selection);
  col_index = ligma_colormap_selection_get_index (selection, NULL);

  op = (LigmaChannelOps) g_variant_get_int32 (value);

  drawables = ligma_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      /* We should not reach this anyway as colormap-actions.c normally takes
       * care at making the action insensitive when the item selection is wrong.
       */
      g_warning ("This action requires exactly one selected drawable.");
      g_list_free (drawables);
      return;
    }

  ligma_channel_select_by_index (ligma_image_get_mask (image),
                                drawables->data,
                                col_index, op,
                                FALSE, 0.0, 0.0);

  g_list_free (drawables);
  ligma_image_flush (image);
}
