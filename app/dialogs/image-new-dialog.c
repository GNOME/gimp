/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimptemplate.h"

#include "widgets/gimpcontainermenuimpl.h"
#include "widgets/gimptemplateeditor.h"
#include "widgets/gimpviewabledialog.h"

#include "file-new-dialog.h"

#include "gimp-intl.h"


typedef struct
{
  GtkWidget    *dialog;
  GtkWidget    *confirm_dialog;

  GtkWidget    *template_menu;
  GtkWidget    *editor;
  GtkWidget    *ok_button;

  Gimp         *gimp;
  gulong        memsize;
} FileNewDialog;


/*  local function prototypes  */

static void   file_new_ok_callback     (GtkWidget         *widget,
                                        FileNewDialog     *dialog);
static void   file_new_cancel_callback (GtkWidget         *widget,
                                        FileNewDialog     *dialog);
static void   file_new_reset_callback  (GtkWidget         *widget,
                                        FileNewDialog     *dialog);
static void   file_new_template_notify (GimpTemplate      *template,
                                        GParamSpec        *param_spec,
                                        FileNewDialog     *dialog);
static void   file_new_template_select (GimpContainerMenu *menu,
                                        GimpTemplate      *template,
                                        gpointer           insert_data,
                                        FileNewDialog     *dialog);
static void   file_new_confirm_dialog  (FileNewDialog     *dialog);
static void   file_new_create_image    (FileNewDialog     *dialog);


/*  public functions  */

void
file_new_dialog_create (Gimp         *gimp,
                        GimpImage    *gimage,
                        GimpTemplate *template)
{
  FileNewDialog *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *table;
  GtkWidget     *optionmenu;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimage == NULL || GIMP_IS_IMAGE (gimage));
  g_return_if_fail (template == NULL || GIMP_IS_TEMPLATE (template));

  dialog = g_new0 (FileNewDialog, 1);

  dialog->gimp    = gimp;
  dialog->memsize = 0;

  dialog->dialog =
    gimp_viewable_dialog_new (NULL,
                              _("New Image"), "new_image",
                              GIMP_STOCK_IMAGE,
                              _("Create a New Image"),
                              gimp_standard_help_func,
                              "dialogs/file_new.html",

                              GIMP_STOCK_RESET, file_new_reset_callback,
                              dialog, NULL, NULL, FALSE, FALSE,

                              GTK_STOCK_CANCEL, file_new_cancel_callback,
                              dialog, NULL, NULL, FALSE, TRUE,

                              GTK_STOCK_OK, file_new_ok_callback,
                              dialog, NULL, &dialog->ok_button, TRUE, FALSE,

                              NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

  /*  vbox holding the rest of the dialog  */
  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
		      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu = gtk_option_menu_new ();

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("From _Template:"),  1.0, 0.5,
                             optionmenu, 1, FALSE);

  dialog->template_menu = gimp_container_menu_new (gimp->templates, NULL, 16, 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), dialog->template_menu);
  gtk_widget_show (dialog->template_menu);

  gimp_container_menu_select_item (GIMP_CONTAINER_MENU (dialog->template_menu),
                                   NULL);

  g_signal_connect (dialog->template_menu, "select_item",
                    G_CALLBACK (file_new_template_select),
                    dialog);

  /*  Template editor  */
  dialog->editor = gimp_template_editor_new (gimp, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), dialog->editor, FALSE, FALSE, 0);
  gtk_widget_show (dialog->editor);

  g_signal_connect (GIMP_TEMPLATE_EDITOR (dialog->editor)->template, "notify",
                    G_CALLBACK (file_new_template_notify),
                    dialog);

  if (template)
    {
      gimp_container_menu_select_item (GIMP_CONTAINER_MENU (dialog->template_menu),
                                       GIMP_VIEWABLE (template));
    }
  else
    {
      template = gimp_image_new_get_last_template (gimp, gimage);
      gimp_template_editor_set_template (GIMP_TEMPLATE_EDITOR (dialog->editor),
                                         template);
      g_object_unref (template);
    }

  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (GIMP_TEMPLATE_EDITOR (dialog->editor)->size_se));

  gtk_widget_show (dialog->dialog);
}


/*  private functions  */

static void
file_new_ok_callback (GtkWidget     *widget,
		      FileNewDialog *dialog)
{
  if (dialog->memsize >
      GIMP_GUI_CONFIG (dialog->gimp->config)->max_new_image_size)
    file_new_confirm_dialog (dialog);
  else
    file_new_create_image (dialog);
}

static void
file_new_cancel_callback (GtkWidget     *widget,
			  FileNewDialog *dialog)
{
  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

static void
file_new_reset_callback (GtkWidget     *widget,
			 FileNewDialog *dialog)
{
  GimpTemplate *template;

  template = gimp_template_new ("foo");
  gimp_template_set_from_config (template, dialog->gimp->config);
  gimp_template_editor_set_template (GIMP_TEMPLATE_EDITOR (dialog->editor),
                                     template);
  g_object_unref (template);
}

static void
file_new_template_notify (GimpTemplate  *template,
                          GParamSpec    *param_spec,
                          FileNewDialog *dialog)
{
  if (dialog->memsize != template->initial_size)
    {
      dialog->memsize = template->initial_size;

      gtk_widget_set_sensitive (dialog->ok_button,
                                ! template->initial_size_too_large);
    }
}

static void
file_new_template_select (GimpContainerMenu *menu,
                          GimpTemplate      *template,
                          gpointer           insert_data,
                          FileNewDialog     *dialog)
{
  if (template)
    gimp_template_editor_set_template (GIMP_TEMPLATE_EDITOR (dialog->editor),
                                       template);
}


/*  the confirm dialog  */

static void
file_new_confirm_dialog_callback (GtkWidget *widget,
				  gboolean   create,
				  gpointer   data)
{
  FileNewDialog *dialog = (FileNewDialog *) data;

  dialog->confirm_dialog = NULL;

  if (create)
    file_new_create_image (dialog);
  else
    gtk_widget_set_sensitive (dialog->dialog, TRUE);
}

static void
file_new_confirm_dialog (FileNewDialog *dialog)
{
  gchar *size_str;
  gchar *max_size_str;
  gchar *text;

  size_str     = gimp_memsize_to_string (dialog->memsize);
  max_size_str = gimp_memsize_to_string (GIMP_GUI_CONFIG (dialog->gimp->config)->max_new_image_size);

  text = g_strdup_printf (_("You are trying to create an image with\n"
			    "an initial size of %s.\n\n"
			    "Choose OK to create this image anyway.\n"
			    "Choose Cancel if you did not intend to\n"
			    "create such a large image.\n\n"
			    "To prevent this dialog from appearing,\n"
			    "increase the \"Maximum Image Size\"\n"
			    "setting (currently %s) in the\n"
			    "Preferences dialog."),
                          size_str, max_size_str);

  g_free (size_str);
  g_free (max_size_str);

  dialog->confirm_dialog =
    gimp_query_boolean_box (_("Confirm Image Size"),
			    gimp_standard_help_func,
			    "dialogs/file_new.html#confirm_size",
			    GIMP_STOCK_INFO,
			    text,
			    GTK_STOCK_OK, GTK_STOCK_CANCEL,
			    NULL, NULL,
			    file_new_confirm_dialog_callback,
			    dialog);

  g_free (text);

  gtk_window_set_transient_for (GTK_WINDOW (dialog->confirm_dialog),
				GTK_WINDOW (dialog->dialog));

  gtk_widget_set_sensitive (dialog->dialog, FALSE);

  gtk_widget_show (dialog->confirm_dialog);
}

static void
file_new_create_image (FileNewDialog *dialog)
{
  GimpTemplate *template;

  template =
    gimp_template_editor_get_template (GIMP_TEMPLATE_EDITOR (dialog->editor));

  gtk_widget_destroy (dialog->dialog);

  gimp_template_create_image (dialog->gimp, template);
  gimp_image_new_set_last_template (dialog->gimp, template);
  g_object_unref (template);

  g_free (dialog);
}
