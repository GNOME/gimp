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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptemplate.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimptemplateeditor.h"
#include "widgets/gimptemplateview.h"

#include "dialogs/dialogs.h"
#include "dialogs/image-new-dialog.h"
#include "dialogs/template-options-dialog.h"

#include "templates-commands.h"

#include "gimp-intl.h"


typedef struct
{
  GimpContext   *context;
  GimpContainer *container;
  GimpTemplate  *template;
} TemplateDeleteData;


/*  local function prototypes  */

static void   templates_new_response     (GtkWidget             *dialog,
                                          gint                   response_id,
                                          TemplateOptionsDialog *options);
static void   templates_edit_response    (GtkWidget             *widget,
                                          gint                   response_id,
                                          TemplateOptionsDialog *options);
static void   templates_delete_response  (GtkWidget             *dialog,
                                          gint                   response_id,
                                          TemplateDeleteData    *delete_data);
static void   templates_delete_data_free (TemplateDeleteData    *delete_data);


/*  public functions */

void
templates_create_image_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpTemplate        *template;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  template = gimp_context_get_template (context);

  if (template && gimp_container_have (container, GIMP_OBJECT (template)))
    {
      GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (editor));
      GtkWidget *dialog;

      dialog = gimp_dialog_factory_dialog_new (global_dialog_factory, screen,
                                               "gimp-image-new-dialog",
                                               -1, FALSE);

      if (dialog)
        {
          image_new_dialog_set (dialog, NULL, template);

          gtk_window_present (GTK_WINDOW (dialog));
        }
    }
}

void
templates_new_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpContainerEditor   *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext           *context;
  TemplateOptionsDialog *options;

  context = gimp_container_view_get_context (editor->view);

  options = template_options_dialog_new (NULL, context,
                                         GTK_WIDGET (editor),
                                         _("New Template"),
                                         "gimp-template-new",
                                         GIMP_STOCK_TEMPLATE,
                                         _("Create a New Template"),
                                         GIMP_HELP_TEMPLATE_NEW);

  g_signal_connect (options->dialog, "response",
                    G_CALLBACK (templates_new_response),
                    options);

  gtk_widget_show (options->dialog);
}

void
templates_duplicate_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpTemplate        *template;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  template = gimp_context_get_template (context);

  if (template && gimp_container_have (container, GIMP_OBJECT (template)))
    {
      GimpTemplate *new_template;

      new_template = gimp_config_duplicate (GIMP_CONFIG (template));

      gimp_container_add (container, GIMP_OBJECT (new_template));
      gimp_context_set_by_type (context, container->children_type,
                                GIMP_OBJECT (new_template));
      g_object_unref (new_template);

      templates_edit_cmd_callback (action, data);
    }
}

void
templates_edit_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpTemplate        *template;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  template = gimp_context_get_template (context);

  if (template && gimp_container_have (container, GIMP_OBJECT (template)))
    {
      TemplateOptionsDialog *options;

      options = template_options_dialog_new (template, context,
                                             GTK_WIDGET (editor),
                                             _("Edit Template"),
                                             "gimp-template-edit",
                                             GTK_STOCK_EDIT,
                                             _("Edit Template"),
                                             GIMP_HELP_TEMPLATE_EDIT);

      g_signal_connect (options->dialog, "response",
                        G_CALLBACK (templates_edit_response),
                        options);

      gtk_widget_show (options->dialog);
    }
}

void
templates_delete_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpTemplate        *template;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  template = gimp_context_get_template (context);

  if (template && gimp_container_have (container, GIMP_OBJECT (template)))
    {
      TemplateDeleteData *delete_data = g_slice_new (TemplateDeleteData);
      GtkWidget          *dialog;

      delete_data->context   = context;
      delete_data->container = container;
      delete_data->template  = template;

      dialog =
        gimp_message_dialog_new (_("Delete Template"), GIMP_STOCK_QUESTION,
                                 GTK_WIDGET (editor), 0,
                                 gimp_standard_help_func, NULL,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_DELETE, GTK_RESPONSE_OK,

                                 NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

      gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                         _("Are you sure you want to delete "
                                           "template '%s' from the list and "
                                           "from disk?"),
                                         GIMP_OBJECT (template)->name);
      gtk_widget_show (dialog);
    }
}


/*  private functions  */

static void
templates_new_response (GtkWidget             *dialog,
                        gint                   response_id,
                        TemplateOptionsDialog *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpTemplateEditor *editor   = GIMP_TEMPLATE_EDITOR (options->editor);
      GimpTemplate       *template = editor->template;

      gimp_container_add (options->gimp->templates, GIMP_OBJECT (template));
      gimp_context_set_template (gimp_get_user_context (options->gimp),
                                 template);
    }

  gtk_widget_destroy (dialog);
}

static void
templates_edit_response (GtkWidget             *dialog,
                         gint                   response_id,
                         TemplateOptionsDialog *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpTemplateEditor *editor = GIMP_TEMPLATE_EDITOR (options->editor);

      gimp_config_sync (G_OBJECT (editor->template),
                        G_OBJECT (options->template), 0);
    }

  gtk_widget_destroy (dialog);
}

static void
templates_delete_response (GtkWidget          *dialog,
                           gint                response_id,
                           TemplateDeleteData *delete_data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpObject *new_active;

      new_active = gimp_container_get_neighbor_of_active (delete_data->container,
                                                          delete_data->context,
                                                          GIMP_OBJECT (delete_data->template));

      if (gimp_container_have (delete_data->container,
                               GIMP_OBJECT (delete_data->template)))
        {
          if (new_active)
            gimp_context_set_by_type (delete_data->context,
                                      delete_data->container->children_type,
                                      new_active);

          gimp_container_remove (delete_data->container,
                                 GIMP_OBJECT (delete_data->template));
        }
    }

  gtk_widget_destroy (dialog);
}

static void
templates_delete_data_free (TemplateDeleteData *delete_data)
{
  g_slice_free (TemplateDeleteData, delete_data);
}
