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

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage-new.h"
#include "core/ligmatemplate.h"

#include "widgets/ligmacontainerview.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmatemplateeditor.h"
#include "widgets/ligmatemplateview.h"
#include "widgets/ligmawidgets-utils.h"

#include "dialogs/dialogs.h"
#include "dialogs/template-options-dialog.h"

#include "actions.h"
#include "templates-commands.h"

#include "ligma-intl.h"


typedef struct
{
  LigmaContext   *context;
  LigmaContainer *container;
  LigmaTemplate  *template;
} TemplateDeleteData;


/*  local function prototypes  */

static void   templates_new_callback     (GtkWidget          *dialog,
                                          LigmaTemplate       *template,
                                          LigmaTemplate       *edit_template,
                                          LigmaContext        *context,
                                          gpointer            user_data);
static void   templates_edit_callback    (GtkWidget          *dialog,
                                          LigmaTemplate       *template,
                                          LigmaTemplate       *edit_template,
                                          LigmaContext        *context,
                                          gpointer            user_data);
static void   templates_delete_response  (GtkWidget          *dialog,
                                          gint                response_id,
                                          TemplateDeleteData *delete_data);
static void   templates_delete_data_free (TemplateDeleteData *delete_data);


/*  public functions */

void
templates_create_image_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  Ligma                *ligma;
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaTemplate        *template;
  return_if_no_ligma (ligma, data);

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  template = ligma_context_get_template (context);

  if (template && ligma_container_have (container, LIGMA_OBJECT (template)))
    {
      GtkWidget *widget = GTK_WIDGET (editor);
      LigmaImage *image;

      image = ligma_image_new_from_template (ligma, template, context);
      ligma_create_display (ligma, image, ligma_template_get_unit (template), 1.0,
                           G_OBJECT (ligma_widget_get_monitor (widget)));
      g_object_unref (image);

      ligma_image_new_set_last_template (ligma, template);
    }
}

void
templates_new_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context;
  GtkWidget           *dialog;

  context = ligma_container_view_get_context (editor->view);

#define NEW_DIALOG_KEY "ligma-template-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (context->ligma), NEW_DIALOG_KEY);

  if (! dialog)
    {
      dialog = template_options_dialog_new (NULL, context,
                                            GTK_WIDGET (editor),
                                            _("New Template"),
                                            "ligma-template-new",
                                            LIGMA_ICON_TEMPLATE,
                                            _("Create a New Template"),
                                            LIGMA_HELP_TEMPLATE_NEW,
                                            templates_new_callback,
                                            NULL);

      dialogs_attach_dialog (G_OBJECT (context->ligma), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
templates_duplicate_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaTemplate        *template;

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  template = ligma_context_get_template (context);

  if (template && ligma_container_have (container, LIGMA_OBJECT (template)))
    {
      LigmaTemplate *new_template;

      new_template = ligma_config_duplicate (LIGMA_CONFIG (template));

      ligma_container_add (container, LIGMA_OBJECT (new_template));
      ligma_context_set_by_type (context,
                                ligma_container_get_children_type (container),
                                LIGMA_OBJECT (new_template));
      g_object_unref (new_template);

      templates_edit_cmd_callback (action, value, data);
    }
}

void
templates_edit_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaTemplate        *template;

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  template = ligma_context_get_template (context);

  if (template && ligma_container_have (container, LIGMA_OBJECT (template)))
    {
      GtkWidget *dialog;

#define EDIT_DIALOG_KEY "ligma-template-edit-dialog"

      dialog = dialogs_get_dialog (G_OBJECT (template), EDIT_DIALOG_KEY);

      if (! dialog)
        {
          dialog = template_options_dialog_new (template, context,
                                                GTK_WIDGET (editor),
                                                _("Edit Template"),
                                                "ligma-template-edit",
                                                LIGMA_ICON_EDIT,
                                                _("Edit Template"),
                                                LIGMA_HELP_TEMPLATE_EDIT,
                                                templates_edit_callback,
                                                NULL);

          dialogs_attach_dialog (G_OBJECT (template), EDIT_DIALOG_KEY, dialog);
        }

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

void
templates_delete_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaTemplate        *template;

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  template = ligma_context_get_template (context);

  if (template && ligma_container_have (container, LIGMA_OBJECT (template)))
    {
      TemplateDeleteData *delete_data = g_slice_new (TemplateDeleteData);
      GtkWidget          *dialog;

      delete_data->context   = context;
      delete_data->container = container;
      delete_data->template  = template;

      dialog =
        ligma_message_dialog_new (_("Delete Template"), "edit-delete",
                                 GTK_WIDGET (editor), 0,
                                 ligma_standard_help_func, NULL,

                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                 _("_Delete"), GTK_RESPONSE_OK,

                                 NULL);

      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_object_weak_ref (G_OBJECT (dialog),
                         (GWeakNotify) templates_delete_data_free, delete_data);

      g_signal_connect_object (template, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (templates_delete_response),
                        delete_data);

      ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                         _("Are you sure you want to delete "
                                           "template '%s' from the list and "
                                           "from disk?"),
                                         ligma_object_get_name (template));
      gtk_widget_show (dialog);
    }
}


/*  private functions  */

static void
templates_new_callback (GtkWidget    *dialog,
                        LigmaTemplate *template,
                        LigmaTemplate *edit_template,
                        LigmaContext  *context,
                        gpointer      user_data)
{
  ligma_container_add (context->ligma->templates, LIGMA_OBJECT (edit_template));
  ligma_context_set_template (ligma_get_user_context (context->ligma),
                             edit_template);

  gtk_widget_destroy (dialog);
}

static void
templates_edit_callback (GtkWidget    *dialog,
                         LigmaTemplate *template,
                         LigmaTemplate *edit_template,
                         LigmaContext  *context,
                         gpointer      user_data)
{
  ligma_config_sync (G_OBJECT (edit_template),
                    G_OBJECT (template), 0);

  gtk_widget_destroy (dialog);
}

static void
templates_delete_response (GtkWidget          *dialog,
                           gint                response_id,
                           TemplateDeleteData *delete_data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      LigmaObject *new_active = NULL;

      if (delete_data->template ==
          ligma_context_get_template (delete_data->context))
        {
          new_active = ligma_container_get_neighbor_of (delete_data->container,
                                                       LIGMA_OBJECT (delete_data->template));
        }

      if (ligma_container_have (delete_data->container,
                               LIGMA_OBJECT (delete_data->template)))
        {
          if (new_active)
            ligma_context_set_by_type (delete_data->context,
                                      ligma_container_get_children_type (delete_data->container),
                                      new_active);

          ligma_container_remove (delete_data->container,
                                 LIGMA_OBJECT (delete_data->template));
        }
    }

  gtk_widget_destroy (dialog);
}

static void
templates_delete_data_free (TemplateDeleteData *delete_data)
{
  g_slice_free (TemplateDeleteData, delete_data);
}
