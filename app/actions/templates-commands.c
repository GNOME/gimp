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
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptemplate.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimptemplateeditor.h"
#include "widgets/gimptemplateview.h"
#include "widgets/gimpviewabledialog.h"

#include "gui/dialogs.h"
#include "gui/file-new-dialog.h"

#include "templates-commands.h"

#include "gimp-intl.h"


/*  public functions */

void
templates_new_template_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpTemplateView *view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->new_button));
}

void
templates_duplicate_template_cmd_callback (GtkWidget *widget,
                                           gpointer   data)
{
  GimpTemplateView *view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->duplicate_button));
}

void
templates_edit_template_cmd_callback (GtkWidget *widget,
                                      gpointer   data)
{
  GimpTemplateView *view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->edit_button));
}

void
templates_create_image_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpTemplateView *view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->create_button));
}

void
templates_delete_template_cmd_callback (GtkWidget *widget,
                                        gpointer   data)
{
  GimpTemplateView *view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->delete_button));
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

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
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

      gimp_list_uniquefy_name (GIMP_LIST (gimp->templates),
                               GIMP_OBJECT (template), TRUE);
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

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
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
templates_file_new_dialog (Gimp         *gimp,
                           GimpTemplate *template,
                           GtkWidget    *parent)
{
  GtkWidget *dialog;

  dialog = gimp_dialog_factory_dialog_new (global_dialog_factory,
                                           gtk_widget_get_screen (parent),
                                           "gimp-file-new-dialog", -1);

  if (dialog)
    file_new_dialog_set (dialog, NULL, template);
}
