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

#include "gui-types.h"

#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptemplate.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimptemplateeditor.h"
#include "widgets/gimptemplateview.h"
#include "widgets/gimpviewabledialog.h"

#include "dialogs.h"
#include "file-new-dialog.h"
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
templates_new_template_ok_callback (GtkWidget *widget,
                                    GtkWidget *dialog)
{
  GimpTemplateEditor *editor;
  GimpTemplate       *template;
  Gimp               *gimp;

  editor   = g_object_get_data (G_OBJECT (dialog), "gimp-template-editor");
  template = g_object_get_data (G_OBJECT (dialog), "gimp-template");
  gimp     = g_object_get_data (G_OBJECT (dialog), "gimp");

  gimp_config_copy_properties (G_OBJECT (editor->template),
                               G_OBJECT (template));

  gimp_list_uniquefy_name (GIMP_LIST (gimp->templates),
                           GIMP_OBJECT (template), TRUE);

  gimp_container_add (gimp->templates, GIMP_OBJECT (template));
  gimp_context_set_template (gimp_get_user_context (gimp), template);

  gtk_widget_destroy (dialog);
}

void
templates_new_template_dialog (Gimp         *gimp,
                               GimpTemplate *template_template)
{
  GimpTemplate *template;
  GtkWidget    *dialog;
  GtkWidget    *main_vbox;
  GtkWidget    *editor;

  template = gimp_template_new (_("Unnamed"));

  gimp_template_set_from_config (template, gimp->config);

  dialog =
    gimp_viewable_dialog_new (NULL,
                              _("New Template"), "new_template",
                              GIMP_STOCK_TEMPLATE,
                              _("Create a New Template"),
                              gimp_standard_help_func,
                              GIMP_HELP_TEMPLATE_NEW,

                              GTK_STOCK_CANCEL, gtk_widget_destroy,
                              NULL, (gpointer) 1, NULL, FALSE, TRUE,

                              GTK_STOCK_OK,
                              G_CALLBACK (templates_new_template_ok_callback),
                              NULL, NULL, NULL, TRUE, FALSE,

                              NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  editor = gimp_template_editor_new (gimp, TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), editor, FALSE, FALSE, 0);
  gtk_widget_show (editor);

  gimp_template_editor_set_template (GIMP_TEMPLATE_EDITOR (editor),
                                     template);

  g_object_set_data (G_OBJECT (dialog), "gimp-template-editor", editor);
  g_object_set_data (G_OBJECT (dialog), "gimp",                 gimp);

  g_object_set_data_full (G_OBJECT (dialog), "gimp-template", template,
                          (GDestroyNotify) g_object_unref);

  gtk_widget_show (dialog);
}

static void
templates_edit_template_ok_callback (GtkWidget *widget,
                                     GtkWidget *dialog)
{
  GimpTemplateEditor *editor;
  GimpTemplate       *template;
  Gimp               *gimp;

  editor   = g_object_get_data (G_OBJECT (dialog), "gimp-template-editor");
  template = g_object_get_data (G_OBJECT (dialog), "gimp-template");
  gimp     = g_object_get_data (G_OBJECT (dialog), "gimp");

  gimp_config_copy_properties (G_OBJECT (editor->template),
                               G_OBJECT (template));
  gimp_list_uniquefy_name (GIMP_LIST (gimp->templates),
                           GIMP_OBJECT (template), TRUE);

  gtk_widget_destroy (dialog);
}

void
templates_edit_template_dialog (Gimp         *gimp,
                                GimpTemplate *template)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *editor;

  dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (template),
                              _("Edit Template"), "edit_template",
                              GIMP_STOCK_EDIT,
                              _("Edit Template"),
                              gimp_standard_help_func,
                              GIMP_HELP_TEMPLATE_EDIT,

                              GTK_STOCK_CANCEL, gtk_widget_destroy,
                              NULL, (gpointer) 1, NULL, FALSE, TRUE,

                              GTK_STOCK_OK,
                              G_CALLBACK (templates_edit_template_ok_callback),
                              NULL, NULL, NULL, TRUE, FALSE,

                              NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  editor = gimp_template_editor_new (gimp, TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), editor, FALSE, FALSE, 0);
  gtk_widget_show (editor);

  gimp_template_editor_set_template (GIMP_TEMPLATE_EDITOR (editor),
                                     template);

  g_object_set_data (G_OBJECT (dialog), "gimp-template-editor", editor);
  g_object_set_data (G_OBJECT (dialog), "gimp-template",        template);
  g_object_set_data (G_OBJECT (dialog), "gimp",                 gimp);

  gtk_widget_show (dialog);
}

void
templates_file_new_dialog (Gimp         *gimp,
                           GimpTemplate *template)
{
  GtkWidget *dialog;

  dialog = gimp_dialog_factory_dialog_new (global_dialog_factory,
                                           "gimp-file-new-dialog", -1);

  if (dialog)
    file_new_dialog_set (dialog, NULL, template);
}
