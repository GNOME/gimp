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

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadata.h"
#include "core/ligmadatafactory.h"

#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"

#include "data-delete-dialog.h"

#include "ligma-intl.h"


typedef struct _DataDeleteDialog DataDeleteDialog;

struct _DataDeleteDialog
{
  LigmaDataFactory *factory;
  LigmaData        *data;
  LigmaContext     *context;
  GtkWidget       *parent;
};


/*  local function prototypes  */

static void  data_delete_dialog_response (GtkWidget        *dialog,
                                          gint              response_id,
                                          DataDeleteDialog *private);


/*  public functions  */

GtkWidget *
data_delete_dialog_new (LigmaDataFactory *factory,
                        LigmaData        *data,
                        LigmaContext     *context,
                        GtkWidget       *parent)
{
  DataDeleteDialog *private;
  GtkWidget        *dialog;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_DATA (data), NULL);
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  private = g_slice_new0 (DataDeleteDialog);

  private->factory = factory;
  private->data    = data;
  private->context = context;
  private->parent  = parent;

  dialog = ligma_message_dialog_new (_("Delete Object"), "edit-delete",
                                    gtk_widget_get_toplevel (parent), 0,
                                    ligma_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Delete"), GTK_RESPONSE_OK,

                                    NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (data, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (data_delete_dialog_response),
                    private);

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     _("Delete '%s'?"),
                                     ligma_object_get_name (data));
  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                             _("Are you sure you want to remove '%s' "
                               "from the list and delete it on disk?"),
                             ligma_object_get_name (data));

  return dialog;
}


/*  private functions  */

static void
data_delete_dialog_response (GtkWidget        *dialog,
                             gint              response_id,
                             DataDeleteDialog *private)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      LigmaDataFactory *factory    = private->factory;
      LigmaData        *data       = private->data;
      LigmaContainer   *container;
      LigmaObject      *new_active = NULL;
      GError          *error      = NULL;

      container = ligma_data_factory_get_container (factory);

      if (private->context &&
          LIGMA_OBJECT (data) ==
          ligma_context_get_by_type (private->context,
                                    ligma_container_get_children_type (container)))
        {
          new_active = ligma_container_get_neighbor_of (container,
                                                       LIGMA_OBJECT (data));
        }

      if (! ligma_data_factory_data_delete (factory, data, TRUE, &error))
        {
          ligma_message (ligma_data_factory_get_ligma (factory),
                        G_OBJECT (private->parent), LIGMA_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }

      if (new_active)
        ligma_context_set_by_type (private->context,
                                  ligma_container_get_children_type (container),
                                  new_active);
    }

  g_slice_free (DataDeleteDialog, private);
}
