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
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimppalette.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpview.h"
#include "widgets/gimpwidgets-utils.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "palettes-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   palettes_merge_callback (GtkWidget   *widget,
                                       const gchar *palette_name,
                                       gpointer     data);


/*  public functions */

void
palettes_import_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                  gimp_widget_get_monitor (widget),
                                  NULL /*ui_manager*/,
                                  "gimp-palette-import-dialog", -1, TRUE);
}

void
palettes_merge_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GtkWidget           *dialog;

#define MERGE_DIALOG_KEY "gimp-palettes-merge-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (editor), MERGE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = gimp_query_string_box (_("Merge Palettes"),
                                      GTK_WIDGET (editor),
                                      gimp_standard_help_func,
                                      GIMP_HELP_PALETTE_MERGE,
                                      _("Enter a name for the merged palette"),
                                      NULL,
                                      G_OBJECT (editor), "destroy",
                                      palettes_merge_callback, editor);

      dialogs_attach_dialog (G_OBJECT (editor), MERGE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
palettes_merge_callback (GtkWidget   *widget,
                         const gchar *palette_name,
                         gpointer     data)
{
  GimpContainerEditor *editor = data;
  GimpDataFactoryView *view   = data;
  GimpDataFactory     *factory;
  GimpContext         *context;
  GimpPalette         *new_palette;
  GList               *selected = NULL;
  GList               *list;

  context = gimp_container_view_get_context (editor->view);
  factory = gimp_data_factory_view_get_data_factory (view);

  gimp_container_view_get_selected (editor->view, &selected);

  if (g_list_length (selected) < 2)
    {
      gimp_message_literal (context->gimp,
                            G_OBJECT (editor), GIMP_MESSAGE_WARNING,
                            _("There must be at least two palettes selected "
                              "to merge."));
      g_list_free (selected);
      return;
    }

  new_palette = GIMP_PALETTE (gimp_data_factory_data_new (factory, context,
                                                          palette_name));

  for (list = selected; list; list = g_list_next (list))
    {
      GimpPalette *palette = list->data;
      GList       *cols;

      for (cols = gimp_palette_get_colors (palette);
           cols;
           cols = g_list_next (cols))
        {
          GimpPaletteEntry *entry = cols->data;

          gimp_palette_add_entry (new_palette, -1,
                                  entry->name,
                                  &entry->color);
        }
    }

  g_list_free (selected);
}
