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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligma-filter-history.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaitem.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmaprogress.h"

#include "pdb/ligmaprocedure.h"

#include "plug-in/ligmapluginmanager.h"
#include "plug-in/ligmapluginmanager-data.h"

#include "widgets/ligmabufferview.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmadatafactoryview.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaimageeditor.h"
#include "widgets/ligmaitemtreeview.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "plug-in-commands.h"
#include "procedure-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   plug_in_reset_all_response (GtkWidget *dialog,
                                          gint       response_id,
                                          Ligma      *ligma);


/*  public functions  */

void
plug_in_run_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  Ligma           *ligma;
  LigmaValueArray *args    = NULL;
  LigmaDisplay    *display = NULL;
  LigmaProcedure  *procedure;
  gsize           hack;
  return_if_no_ligma (ligma, data);

  hack = g_variant_get_uint64 (value);

  procedure = GSIZE_TO_POINTER (hack);

  switch (procedure->proc_type)
    {
    case LIGMA_PDB_PROC_TYPE_EXTENSION:
      args = procedure_commands_get_run_mode_arg (procedure);
      break;

    case LIGMA_PDB_PROC_TYPE_PLUGIN:
    case LIGMA_PDB_PROC_TYPE_TEMPORARY:
      if (LIGMA_IS_DATA_FACTORY_VIEW (data) ||
          LIGMA_IS_BUFFER_VIEW (data))
        {
          LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
          LigmaContainer       *container;
          LigmaContext         *context;
          LigmaObject          *object;

          container = ligma_container_view_get_container (editor->view);
          context   = ligma_container_view_get_context (editor->view);

          object = ligma_context_get_by_type (context,
                                             ligma_container_get_children_type (container));

          args = procedure_commands_get_data_args (procedure, object);
        }
      else if (LIGMA_IS_IMAGE_EDITOR (data))
        {
          LigmaImageEditor *editor = LIGMA_IMAGE_EDITOR (data);
          LigmaImage       *image;

          image = ligma_image_editor_get_image (editor);

          args = procedure_commands_get_image_args (procedure, image);
        }
      else if (LIGMA_IS_ITEM_TREE_VIEW (data))
        {
          LigmaItemTreeView *view = LIGMA_ITEM_TREE_VIEW (data);
          LigmaImage        *image;
          GList            *items;

          image = ligma_item_tree_view_get_image (view);

          if (image)
            items = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (image);
          else
            items = NULL;

          args = procedure_commands_get_items_args (procedure, image, items);
        }
      else
        {
          display = action_data_get_display (data);

          args = procedure_commands_get_display_args (procedure, display, NULL);
        }
      break;

    case LIGMA_PDB_PROC_TYPE_INTERNAL:
      g_warning ("Unhandled procedure type.");
      break;
    }

  if (args)
    {
      if (procedure_commands_run_procedure_async (procedure, ligma,
                                                  LIGMA_PROGRESS (display),
                                                  LIGMA_RUN_INTERACTIVE, args,
                                                  display))
        {
          /* remember only image plug-ins */
          if (procedure->num_args >= 2 &&
              LIGMA_IS_PARAM_SPEC_IMAGE (procedure->args[1]))
            {
              ligma_filter_history_add (ligma, procedure);
            }
        }

      ligma_value_array_unref (args);
    }
}

void
plug_in_reset_all_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  Ligma      *ligma;
  GtkWidget *dialog;
  return_if_no_ligma (ligma, data);

#define RESET_FILTERS_DIALOG_KEY "ligma-reset-all-filters-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (ligma), RESET_FILTERS_DIALOG_KEY);

  if (! dialog)
    {
      dialog = ligma_message_dialog_new (_("Reset all Filters"),
                                        LIGMA_ICON_DIALOG_QUESTION,
                                        NULL, 0,
                                        ligma_standard_help_func, NULL,

                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Reset"),  GTK_RESPONSE_OK,

                                        NULL);

      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (plug_in_reset_all_response),
                        ligma);

      ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                         _("Do you really want to reset all "
                                           "filters to default values?"));

      dialogs_attach_dialog (G_OBJECT (ligma), RESET_FILTERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
plug_in_reset_all_response (GtkWidget *dialog,
                            gint       response_id,
                            Ligma      *ligma)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    ligma_plug_in_manager_data_free (ligma->plug_in_manager);
}
