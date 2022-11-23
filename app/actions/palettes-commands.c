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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmapalette.h"

#include "widgets/ligmacontainerview.h"
#include "widgets/ligmadatafactoryview.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaview.h"
#include "widgets/ligmawidgets-utils.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "palettes-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   palettes_merge_callback (GtkWidget   *widget,
                                       const gchar *palette_name,
                                       gpointer     data);


/*  public functions */

void
palettes_import_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                  ligma_widget_get_monitor (widget),
                                  NULL /*ui_manager*/,
                                  widget,
                                  "ligma-palette-import-dialog", -1, TRUE);
}

void
palettes_merge_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  GtkWidget           *dialog;

#define MERGE_DIALOG_KEY "ligma-palettes-merge-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (editor), MERGE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = ligma_query_string_box (_("Merge Palettes"),
                                      GTK_WIDGET (editor),
                                      ligma_standard_help_func,
                                      LIGMA_HELP_PALETTE_MERGE,
                                      _("Enter a name for the merged palette"),
                                      NULL,
                                      G_OBJECT (editor), "destroy",
                                      palettes_merge_callback,
                                      editor, NULL);

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
  LigmaContainerEditor *editor = data;
  LigmaDataFactoryView *view   = data;
  LigmaDataFactory     *factory;
  LigmaContext         *context;
  LigmaPalette         *new_palette;
  GList               *selected = NULL;
  GList               *list;

  context = ligma_container_view_get_context (editor->view);
  factory = ligma_data_factory_view_get_data_factory (view);

  ligma_container_view_get_selected (editor->view, &selected, NULL);

  if (g_list_length (selected) < 2)
    {
      ligma_message_literal (context->ligma,
                            G_OBJECT (editor), LIGMA_MESSAGE_WARNING,
                            _("There must be at least two palettes selected "
                              "to merge."));
      g_list_free (selected);
      return;
    }

  new_palette = LIGMA_PALETTE (ligma_data_factory_data_new (factory, context,
                                                          palette_name));

  for (list = selected; list; list = g_list_next (list))
    {
      LigmaPalette *palette = list->data;
      GList       *cols;

      for (cols = ligma_palette_get_colors (palette);
           cols;
           cols = g_list_next (cols))
        {
          LigmaPaletteEntry *entry = cols->data;

          ligma_palette_add_entry (new_palette, -1,
                                  entry->name,
                                  &entry->color);
        }
    }

  g_list_free (selected);
}
