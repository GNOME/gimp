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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpconfig-utils.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimptemplate.h"

#include "widgets/gimpcontainermenuimpl.h"
#include "widgets/gimpenummenu.h"
#include "widgets/gimppropwidgets.h"
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
} NewImageInfo;


/*  local function prototypes  */

static void   file_new_ok_callback     (GtkWidget         *widget,
                                        NewImageInfo      *info);
static void   file_new_cancel_callback (GtkWidget         *widget,
                                        NewImageInfo      *info);
static void   file_new_reset_callback  (GtkWidget         *widget,
                                        NewImageInfo      *info);
static void   file_new_template_notify (GimpTemplate      *template,
                                        GParamSpec        *param_spec,
                                        NewImageInfo      *info);
static void   file_new_template_select (GimpContainerMenu *menu,
                                        GimpTemplate      *template,
                                        gpointer           insert_data,
                                        NewImageInfo      *info);
static void   file_new_confirm_dialog  (NewImageInfo      *info);
static void   file_new_create_image    (NewImageInfo      *info);


/*  public functions  */

void
file_new_dialog_create (Gimp      *gimp,
                        GimpImage *gimage)
{
  NewImageInfo *info;
  GimpTemplate *template;
  GtkWidget    *main_vbox;
  GtkWidget    *table;
  GtkWidget    *optionmenu;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimage == NULL || GIMP_IS_IMAGE (gimage));

  info = g_new0 (NewImageInfo, 1);

  info->gimp    = gimp;
  info->memsize = 0;

  info->dialog =
    gimp_viewable_dialog_new (NULL,
                              _("New Image"), "new_image",
                              GIMP_STOCK_IMAGE,
                              _("Create a New Image"),
                              gimp_standard_help_func,
                              "dialogs/file_new.html",

                              GIMP_STOCK_RESET, file_new_reset_callback,
                              info, NULL, NULL, FALSE, FALSE,

                              GTK_STOCK_CANCEL, file_new_cancel_callback,
                              info, NULL, NULL, FALSE, TRUE,

                              GTK_STOCK_OK, file_new_ok_callback,
                              info, NULL, &info->ok_button, TRUE, FALSE,

                              NULL);

  gtk_window_set_resizable (GTK_WINDOW (info->dialog), FALSE);

  /*  vbox holding the rest of the dialog  */
  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info->dialog)->vbox),
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

  info->template_menu = gimp_container_menu_new (gimp->templates, NULL, 16, 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), info->template_menu);
  gtk_widget_show (info->template_menu);

  gimp_container_menu_select_item (GIMP_CONTAINER_MENU (info->template_menu),
                                   NULL);

  g_signal_connect (info->template_menu, "select_item",
                    G_CALLBACK (file_new_template_select),
                    info);

  /*  Template editor  */
  info->editor = gimp_template_editor_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), info->editor, FALSE, FALSE, 0);
  gtk_widget_show (info->editor);

  g_signal_connect (GIMP_TEMPLATE_EDITOR (info->editor)->template, "notify",
                    G_CALLBACK (file_new_template_notify),
                    info);

  template = gimp_image_new_get_last_template (gimp, gimage);
  gimp_template_editor_set_template (GIMP_TEMPLATE_EDITOR (info->editor),
                                     template);
  g_object_unref (template);

  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (GIMP_TEMPLATE_EDITOR (info->editor)->size_se));

  gtk_widget_show (info->dialog);
}


/*  private functions  */

static void
file_new_ok_callback (GtkWidget    *widget,
		      NewImageInfo *info)
{
  if (info->memsize > GIMP_GUI_CONFIG (info->gimp->config)->max_new_image_size)
    file_new_confirm_dialog (info);
  else
    file_new_create_image (info);
}

static void
file_new_cancel_callback (GtkWidget    *widget,
			  NewImageInfo *info)
{
  gtk_widget_destroy (info->dialog);
  g_free (info);
}

static void
file_new_reset_callback (GtkWidget    *widget,
			 NewImageInfo *info)
{
  GimpTemplate *template;

  template = gimp_template_new ("foo");
  gimp_template_set_from_config (template, info->gimp->config);
  gimp_template_editor_set_template (GIMP_TEMPLATE_EDITOR (info->editor),
                                     template);
  g_object_unref (template);
}

static void
file_new_template_notify (GimpTemplate *template,
                          GParamSpec   *param_spec,
                          NewImageInfo *info)
{
  if (info->memsize != template->initial_size)
    {
      info->memsize = template->initial_size;

      gtk_widget_set_sensitive (info->ok_button,
                                ! template->initial_size_too_large);
    }
}

static void
file_new_template_select (GimpContainerMenu *menu,
                          GimpTemplate      *template,
                          gpointer           insert_data,
                          NewImageInfo      *info)
{
  if (template)
    gimp_template_editor_set_template (GIMP_TEMPLATE_EDITOR (info->editor),
                                       template);
}


/*  the confirm dialog  */

static void
file_new_confirm_dialog_callback (GtkWidget *widget,
				  gboolean   create,
				  gpointer   data)
{
  NewImageInfo *info = (NewImageInfo*) data;

  info->confirm_dialog = NULL;

  if (create)
    file_new_create_image (info);
  else
    gtk_widget_set_sensitive (info->dialog, TRUE);
}

static void
file_new_confirm_dialog (NewImageInfo *info)
{
  gchar *size_str;
  gchar *max_size_str;
  gchar *text;

  size_str     = gimp_memsize_to_string (info->memsize);
  max_size_str = gimp_memsize_to_string (GIMP_GUI_CONFIG (info->gimp->config)->max_new_image_size);

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

  info->confirm_dialog =
    gimp_query_boolean_box (_("Confirm Image Size"),
			    gimp_standard_help_func,
			    "dialogs/file_new.html#confirm_size",
			    GIMP_STOCK_INFO,
			    text,
			    GTK_STOCK_OK, GTK_STOCK_CANCEL,
			    NULL, NULL,
			    file_new_confirm_dialog_callback,
			    info);

  g_free (text);

  gtk_window_set_transient_for (GTK_WINDOW (info->confirm_dialog),
				GTK_WINDOW (info->dialog));

  gtk_widget_set_sensitive (info->dialog, FALSE);

  gtk_widget_show (info->confirm_dialog);
}

static void
file_new_create_image (NewImageInfo *info)
{
  GimpTemplate *template;

  template =
    gimp_template_editor_get_template (GIMP_TEMPLATE_EDITOR (info->editor));

  gtk_widget_destroy (info->dialog);

  gimp_template_create_image (info->gimp, template);
  gimp_image_new_set_last_template (info->gimp, template);
  g_object_unref (template);

  g_free (info);
}
