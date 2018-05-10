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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimp-filter-history.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "pdb/gimpprocedure.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginmanager-data.h"

#include "widgets/gimpbufferview.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpfontview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpimageeditor.h"
#include "widgets/gimpitemtreeview.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "plug-in-commands.h"
#include "procedure-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   plug_in_reset_all_response (GtkWidget *dialog,
                                          gint       response_id,
                                          Gimp      *gimp);


/*  public functions  */

void
plug_in_run_cmd_callback (GtkAction     *action,
                          GimpProcedure *procedure,
                          gpointer       data)
{
  Gimp           *gimp;
  GimpValueArray *args    = NULL;
  GimpDisplay    *display = NULL;
  return_if_no_gimp (gimp, data);

  switch (procedure->proc_type)
    {
    case GIMP_EXTENSION:
      args = procedure_commands_get_run_mode_arg (procedure);
      break;

    case GIMP_PLUGIN:
    case GIMP_TEMPORARY:
      if (GIMP_IS_DATA_FACTORY_VIEW (data) ||
          GIMP_IS_FONT_VIEW (data)         ||
          GIMP_IS_BUFFER_VIEW (data))
        {
          GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
          GimpContainer       *container;
          GimpContext         *context;
          GimpObject          *object;

          container = gimp_container_view_get_container (editor->view);
          context   = gimp_container_view_get_context (editor->view);

          object = gimp_context_get_by_type (context,
                                             gimp_container_get_children_type (container));

          args = procedure_commands_get_data_args (procedure, object);
        }
      else if (GIMP_IS_IMAGE_EDITOR (data))
        {
          GimpImageEditor *editor = GIMP_IMAGE_EDITOR (data);
          GimpImage       *image;

          image = gimp_image_editor_get_image (editor);

          args = procedure_commands_get_image_args (procedure, image);
        }
      else if (GIMP_IS_ITEM_TREE_VIEW (data))
        {
          GimpItemTreeView *view = GIMP_ITEM_TREE_VIEW (data);
          GimpImage        *image;
          GimpItem         *item;

          image = gimp_item_tree_view_get_image (view);

          if (image)
            item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);
          else
            item = NULL;

          args = procedure_commands_get_item_args (procedure, image, item);
        }
      else
        {
          display = action_data_get_display (data);

          args = procedure_commands_get_display_args (procedure, display, NULL);
        }
      break;

    case GIMP_INTERNAL:
      g_warning ("Unhandled procedure type.");
      break;
    }

  if (args)
    {
      if (procedure_commands_run_procedure_async (procedure, gimp,
                                                  GIMP_PROGRESS (display),
                                                  GIMP_RUN_INTERACTIVE, args,
                                                  display))
        {
          /* remember only image plug-ins */
          if (procedure->num_args >= 2 &&
              GIMP_IS_PARAM_SPEC_IMAGE_ID (procedure->args[1]))
            {
              gimp_filter_history_add (gimp, procedure);
            }
        }

      gimp_value_array_unref (args);
    }
}

void
plug_in_reset_all_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  Gimp      *gimp;
  GtkWidget *dialog;
  return_if_no_gimp (gimp, data);

#define RESET_FILTERS_DIALOG_KEY "gimp-reset-all-filters-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (gimp), RESET_FILTERS_DIALOG_KEY);

  if (! dialog)
    {
      dialog = gimp_message_dialog_new (_("Reset all Filters"),
                                        GIMP_ICON_DIALOG_QUESTION,
                                        NULL, 0,
                                        gimp_standard_help_func, NULL,

                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Reset"),  GTK_RESPONSE_OK,

                                        NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (plug_in_reset_all_response),
                        gimp);

      gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                         _("Do you really want to reset all "
                                           "filters to default values?"));

      dialogs_attach_dialog (G_OBJECT (gimp), RESET_FILTERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
plug_in_reset_all_response (GtkWidget *dialog,
                            gint       response_id,
                            Gimp      *gimp)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    gimp_plug_in_manager_data_free (gimp->plug_in_manager);
}
