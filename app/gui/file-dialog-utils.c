/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"

#include "plug-in/plug-in-proc.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpmenufactory.h"

#include "file-dialog-utils.h"

#include "gimp-intl.h"


GtkWidget *
file_dialog_new (Gimp              *gimp,
                 GimpDialogFactory *dialog_factory,
                 const gchar       *dialog_identifier,
                 GimpMenuFactory   *menu_factory,
                 const gchar       *menu_identifier,
                 const gchar       *title,
                 const gchar       *role,
                 const gchar       *help_id)
{
  GtkWidget        *filesel;
  GtkFileSelection *fs;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (dialog_identifier != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (menu_identifier != NULL, NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  filesel = gtk_file_selection_new (title);

  fs = GTK_FILE_SELECTION (filesel);

  g_object_set_data (G_OBJECT (filesel), "gimp", gimp);

  gtk_window_set_role (GTK_WINDOW (filesel), role);

  gimp_help_connect (filesel, gimp_standard_help_func, help_id, NULL);

  gtk_container_set_border_width (GTK_CONTAINER (filesel), 6);
  gtk_container_set_border_width (GTK_CONTAINER (fs->button_area), 4);

  g_signal_connect (filesel, "delete_event",
                    G_CALLBACK (gtk_true),
		    NULL);

  /*  The file type menu  */
  {
    GimpItemFactory *item_factory;
    GtkWidget       *hbox;
    GtkWidget       *option_menu;
    GtkWidget       *label;

    hbox = gtk_hbox_new (FALSE, 4);
    gtk_box_pack_end (GTK_BOX (fs->main_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    item_factory = gimp_menu_factory_menu_new (menu_factory,
                                               menu_identifier,
                                               GTK_TYPE_MENU,
                                               gimp,
                                               FALSE);

    g_object_set_data (G_OBJECT (filesel), "gimp-item-factory", item_factory);

    option_menu = gtk_option_menu_new ();
    gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
    gtk_widget_show (option_menu);

    g_object_weak_ref (G_OBJECT (option_menu),
                       (GWeakNotify) g_object_unref,
                       item_factory);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu),
                              GTK_ITEM_FACTORY (item_factory)->widget);

    label = gtk_label_new_with_mnemonic (_("Determine File _Type:"));
    gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), option_menu);
  }

  gimp_dialog_factory_add_foreign (dialog_factory, dialog_identifier, filesel);

  return filesel;
}

void
file_dialog_show (GtkWidget *filesel,
                  GtkWidget *parent)
{
  gimp_item_factories_set_sensitive ("<Toolbox>", "/File/Open...", FALSE);

  gimp_item_factories_set_sensitive ("<Image>", "/File/Open...", FALSE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save", FALSE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save as...", FALSE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save a Copy...", FALSE);

  gtk_window_set_screen (GTK_WINDOW (filesel),
                         gtk_widget_get_screen (parent));

  gtk_widget_grab_focus (GTK_FILE_SELECTION (filesel)->selection_entry);
  gtk_window_present (GTK_WINDOW (filesel));
}

gboolean
file_dialog_hide (GtkWidget *filesel)
{
  gtk_widget_hide (filesel);

  gimp_item_factories_set_sensitive ("<Toolbox>", "/File/Open...", TRUE);

  gimp_item_factories_set_sensitive ("<Image>", "/File/Open...", TRUE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save", TRUE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save as...", TRUE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save a Copy...", TRUE);

  /*  return TRUE because we are used as "delete_event" handler  */
  return TRUE;
}

void
file_dialog_update_name (PlugInProcDef    *proc,
			 GtkFileSelection *filesel)
{
  if (proc->extensions_list)
    {
      const gchar *text;
      gchar       *last_dot;
      GString     *s;

      text = gtk_entry_get_text (GTK_ENTRY (filesel->selection_entry));
      last_dot = strrchr (text, '.');

      if (last_dot == text || !text[0])
	return;

      s = g_string_new (text);

      if (last_dot)
	g_string_truncate (s, last_dot-text);

      g_string_append (s, ".");
      g_string_append (s, (gchar *) proc->extensions_list->data);

      gtk_entry_set_text (GTK_ENTRY (filesel->selection_entry), s->str);

      g_string_free (s, TRUE);
    }
}
