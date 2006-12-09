/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimppalette.h"

#include "widgets/gimpcolordialog.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimppaletteeditor.h"

#include "palette-editor-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void  palette_editor_edit_color_update (GimpColorDialog      *dialog,
                                               const GimpRGB        *color,
                                               GimpColorDialogState  state,
                                               GimpPaletteEditor    *editor);


/*  public functions  */

void
palette_editor_edit_color_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpPaletteEditor *editor      = GIMP_PALETTE_EDITOR (data);
  GimpDataEditor    *data_editor = GIMP_DATA_EDITOR (data);
  GimpPalette       *palette;

  if (! (data_editor->data_editable && editor->color))
    return;

  palette = GIMP_PALETTE (data_editor->data);

  if (! editor->color_dialog)
    {
      editor->color_dialog =
        gimp_color_dialog_new (GIMP_VIEWABLE (palette),
                               data_editor->context,
                               _("Edit Palette Color"),
                               GIMP_STOCK_PALETTE,
                               _("Edit Color Palette Entry"),
                               GTK_WIDGET (editor),
                               gimp_dialog_factory_from_name ("toplevel"),
                               "gimp-palette-editor-color-dialog",
                               &editor->color->color,
                               FALSE, FALSE);

      g_signal_connect (editor->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &editor->color_dialog);

      g_signal_connect (editor->color_dialog, "update",
                        G_CALLBACK (palette_editor_edit_color_update),
                        editor);
    }
  else
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (editor->color_dialog),
                                         GIMP_VIEWABLE (palette),
                                         data_editor->context);
      gimp_color_dialog_set_color (GIMP_COLOR_DIALOG (editor->color_dialog),
                                   &editor->color->color);
    }

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
palette_editor_new_color_cmd_callback (GtkAction *action,
                                       gint       value,
                                       gpointer   data)
{
  GimpPaletteEditor *editor      = GIMP_PALETTE_EDITOR (data);
  GimpDataEditor    *data_editor = GIMP_DATA_EDITOR (data);

  if (data_editor->data_editable)
    {
      GimpPalette *palette = GIMP_PALETTE (data_editor->data);
      GimpRGB      color;

      if (value)
        gimp_context_get_background (data_editor->context, &color);
      else
        gimp_context_get_foreground (data_editor->context, &color);

      editor->color = gimp_palette_add_entry (palette, -1, NULL, &color);
    }
}

void
palette_editor_delete_color_cmd_callback (GtkAction *action,
                                          gpointer   data)
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
palette_editor_zoom_cmd_callback (GtkAction *action,
                                  gint       value,
                                  gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  gimp_palette_editor_zoom (editor, (GimpZoomType) value);
}


/*  private functions  */

static void
palette_editor_edit_color_update (GimpColorDialog      *dialog,
                                  const GimpRGB        *color,
                                  GimpColorDialogState  state,
                                  GimpPaletteEditor    *editor)
{
  GimpPalette *palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case GIMP_COLOR_DIALOG_UPDATE:
      break;

    case GIMP_COLOR_DIALOG_OK:
      if (editor->color)
        {
          editor->color->color = *color;
          gimp_data_dirty (GIMP_DATA (palette));
        }
      /* Fallthrough */

    case GIMP_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (editor->color_dialog);
      break;
    }
}
