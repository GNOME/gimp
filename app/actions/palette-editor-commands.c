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

#include "core/gimpcontext.h"
#include "core/gimppalette.h"

#include "widgets/gimppaletteeditor.h"
#include "widgets/gimppaletteview.h"

#include "palette-editor-commands.h"


/*  public functions  */

void
palette_editor_edit_color_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  gimp_palette_editor_edit_color (editor);
}

void
palette_editor_new_color_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpPaletteEditor *editor      = GIMP_PALETTE_EDITOR (data);
  GimpDataEditor    *data_editor = GIMP_DATA_EDITOR (data);
  gboolean           background  = (gboolean) g_variant_get_int32 (value);

  if (data_editor->data_editable)
    {
      GimpPalette      *palette = GIMP_PALETTE (data_editor->data);
      GimpPaletteEntry *entry;
      GeglColor        *color;

      if (background)
        color = gimp_context_get_background (data_editor->context);
      else
        color = gimp_context_get_foreground (data_editor->context);

      entry = gimp_palette_add_entry (palette, -1, NULL, color);
      gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (editor->view), entry);
    }
}

void
palette_editor_delete_color_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  GimpPaletteEditor *editor      = GIMP_PALETTE_EDITOR (data);
  GimpDataEditor    *data_editor = GIMP_DATA_EDITOR (data);

  if (data_editor->data_editable && editor->color)
    {
      GimpPalette *palette = GIMP_PALETTE (data_editor->data);

      gimp_palette_delete_entry (palette, editor->color);
    }
}

void
palette_editor_zoom_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpPaletteEditor *editor    = GIMP_PALETTE_EDITOR (data);
  GimpZoomType       zoom_type = (GimpZoomType) g_variant_get_int32 (value);

  gimp_palette_editor_zoom (editor, zoom_type);
}
