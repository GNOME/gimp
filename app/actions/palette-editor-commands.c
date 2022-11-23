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

#include "core/ligmacontext.h"
#include "core/ligmapalette.h"

#include "widgets/ligmapaletteeditor.h"
#include "widgets/ligmapaletteview.h"

#include "palette-editor-commands.h"


/*  public functions  */

void
palette_editor_edit_color_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (data);

  ligma_palette_editor_edit_color (editor);
}

void
palette_editor_new_color_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaPaletteEditor *editor      = LIGMA_PALETTE_EDITOR (data);
  LigmaDataEditor    *data_editor = LIGMA_DATA_EDITOR (data);
  gboolean           background  = (gboolean) g_variant_get_int32 (value);

  if (data_editor->data_editable)
    {
      LigmaPalette      *palette = LIGMA_PALETTE (data_editor->data);
      LigmaPaletteEntry *entry;
      LigmaRGB           color;

      if (background)
        ligma_context_get_background (data_editor->context, &color);
      else
        ligma_context_get_foreground (data_editor->context, &color);

      entry = ligma_palette_add_entry (palette, -1, NULL, &color);
      ligma_palette_view_select_entry (LIGMA_PALETTE_VIEW (editor->view), entry);
    }
}

void
palette_editor_delete_color_cmd_callback (LigmaAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  LigmaPaletteEditor *editor      = LIGMA_PALETTE_EDITOR (data);
  LigmaDataEditor    *data_editor = LIGMA_DATA_EDITOR (data);

  if (data_editor->data_editable && editor->color)
    {
      LigmaPalette *palette = LIGMA_PALETTE (data_editor->data);

      ligma_palette_delete_entry (palette, editor->color);
    }
}

void
palette_editor_zoom_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaPaletteEditor *editor    = LIGMA_PALETTE_EDITOR (data);
  LigmaZoomType       zoom_type = (LigmaZoomType) g_variant_get_int32 (value);

  ligma_palette_editor_zoom (editor, zoom_type);
}
