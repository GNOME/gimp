/* The GIMP -- an image manipulation program
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

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"
#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
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
#include "widgets/gimpviewabledialog.h"

#include "dialogs/dialogs.h"
#include "dialogs/image-new-dialog.h"

#include "templates-commands.h"

#include "gimp-intl.h"


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
      templates_image_new_dialog (context->gimp, template, GTK_WIDGET (editor));
    }
}

void
templates_new_template_cmd_callback (GtkAction *action,
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
    }

  templates_new_template_dialog (context->gimp, NULL, GTK_WIDGET (editor));
}

void
templates_duplicate_template_cmd_callback (GtkAction *action,
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

      templates_edit_template_dialog (context->gimp, new_template,
                                      GTK_WIDGET (editor));

      g_object_unref (new_template);
    }
}

void
templates_edit_template_cmd_callback (GtkAction *action,
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
      templates_edit_template_dialog (context->gimp, template,
                                      GTK_WIDGET (editor));
    }
}

typedef struct _GimpTemplateDeleteData GimpTemplateDeleteData;

struct _GimpTemplateDeleteData
{
  GimpContainer *container;
  GimpTemplate  *template;
};

static void
templates_delete_confirm_response (GtkWidget              *dialog,
                                   gint                    response_id,
                                   GimpTemplateDeleteData *delete_data)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      if (gimp_container_have (delete_data->container,
                               GIMP_OBJECT (delete_data->template)))
        {
          gimp_container_remove (delete_data->container,
                                 GIMP_OBJECT (delete_data->template));
        }
    }
}

void
templates_delete_template_cmd_callback (GtkAction *action,
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
      GimpTemplateDeleteData *delete_data;
      GtkWidget              *dialog;

      delete_data = g_new0 (GimpTemplateDeleteData, 1);

      delete_data->container = container;
      delete_data->template  = template;

      dialog =
        gimp_message_dialog_new (_("Delete Template"), GIMP_STOCK_QUESTION,
                                 GTK_WIDGET (editor), 0,
                                 gimp_standard_help_func, NULL,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_DELETE, GTK_RESPONSE_OK,

                                 NULL);

      g_signal_connect_object (template, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (templates_delete_confirm_response),
                        delete_data);

      gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                         _("Are you sure you want to delete "
                                           "template '%s' from the list and "
                                           "from disk?"),
                                         GIMP_OBJECT (template)->name);
      gtk_widget_show (dialog);
    }
}

static void
templates_new_template_response (GtkWidget *widget,
                                 gint       response_id,
                                 GtkWidget *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpTemplate *template;
      Gimp         *gimp;

      template = g_object_get_data (G_OBJECT (dialog), "gimp-template");
      gimp     = g_object_get_data (G_OBJECT (dialog), "gimp");

      gimp_container_add (gimp->templates, GIMP_OBJECT (template));
      gimp_context_set_template (gimp_get_user_context (gimp), template);
    }

  gtk_widget_destroy (dialog);
}

void
templates_new_template_dialog (Gimp         *gimp,
                               GimpTemplate *unused,
                               GtkWidget    *parent)
{
  GimpTemplate *template;
  GtkWidget    *dialog;
  GtkWidget    *main_vbox;
  GtkWidget    *editor;

  dialog = gimp_viewable_dialog_new (NULL,
                                     _("New Template"), "gimp-template-new",
                                     GIMP_STOCK_TEMPLATE,
                                     _("Create a New Template"),
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_TEMPLATE_NEW,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (templates_new_template_response),
                    dialog);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  template = gimp_config_duplicate (GIMP_CONFIG (gimp->config->default_image));
  gimp_object_set_name (GIMP_OBJECT (template), _("Unnamed"));

  editor = gimp_template_editor_new (template, gimp, TRUE);

  g_object_unref (template);

  g_object_set_data (G_OBJECT (dialog), "gimp",          gimp);
  g_object_set_data (G_OBJECT (dialog), "gimp-template", template);

  gtk_box_pack_start (GTK_BOX (main_vbox), editor, FALSE, FALSE, 0);
  gtk_widget_show (editor);

  gtk_widget_show (dialog);
}

static void
templates_edit_template_response (GtkWidget *widget,
                                  gint       response_id,
                                  GtkWidget *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpTemplateEditor *editor;
      GimpTemplate       *template;
      Gimp               *gimp;

      editor   = g_object_get_data (G_OBJECT (dialog), "gimp-template-editor");
      template = g_object_get_data (G_OBJECT (dialog), "gimp-template");
      gimp     = g_object_get_data (G_OBJECT (dialog), "gimp");

      gimp_config_sync (GIMP_CONFIG (editor->template),
                        GIMP_CONFIG (template), 0);
    }

  gtk_widget_destroy (dialog);
}

void
templates_edit_template_dialog (Gimp         *gimp,
                                GimpTemplate *template,
                                GtkWidget    *parent)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *editor;

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (template),
                                     _("Edit Template"), "gimp-template-edit",
                                     GIMP_STOCK_EDIT,
                                     _("Edit Template"),
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_TEMPLATE_EDIT,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (templates_edit_template_response),
                    dialog);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  g_object_set_data (G_OBJECT (dialog), "gimp",          gimp);
  g_object_set_data (G_OBJECT (dialog), "gimp-template", template);

  template = gimp_config_duplicate (GIMP_CONFIG (template));

  editor = gimp_template_editor_new (template, gimp, TRUE);

  g_object_unref (template);

  gtk_box_pack_start (GTK_BOX (main_vbox), editor, FALSE, FALSE, 0);
  gtk_widget_show (editor);

  g_object_set_data (G_OBJECT (dialog), "gimp-template-editor", editor);

  gtk_widget_show (dialog);
}

void
templates_image_new_dialog (Gimp         *gimp,
                            GimpTemplate *template,
                            GtkWidget    *parent)
{
  GtkWidget *dialog;

  dialog = gimp_dialog_factory_dialog_new (global_dialog_factory,
                                           gtk_widget_get_screen (parent),
                                           "gimp-image-new-dialog", -1, FALSE);

  if (dialog)
    {
      image_new_dialog_set (dialog, NULL, template);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}
