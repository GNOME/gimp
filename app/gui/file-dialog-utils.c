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

#include "widgets/gimpitemfactory.h"

#include "file-dialog-utils.h"

#include "libgimp/gimpintl.h"


GtkWidget *
file_dialog_new (Gimp            *gimp,
                 GimpItemFactory *item_factory,
                 const gchar     *title,
                 const gchar     *wmclass_name,
                 const gchar     *help_data,
                 GCallback        ok_callback)
{
  GtkWidget        *filesel;
  GtkFileSelection *fs;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM_FACTORY (item_factory), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (wmclass_name != NULL, NULL);
  g_return_val_if_fail (help_data != NULL, NULL);
  g_return_val_if_fail (ok_callback != NULL, NULL);

  filesel = gtk_file_selection_new (title);

  fs = GTK_FILE_SELECTION (filesel);

  g_object_set_data (G_OBJECT (filesel), "gimp", gimp);

  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (filesel), wmclass_name, "Gimp");

  gimp_help_connect (filesel, gimp_standard_help_func, help_data);

  gtk_container_set_border_width (GTK_CONTAINER (fs->button_area), 2);
  gtk_container_set_border_width (GTK_CONTAINER (filesel), 2);

  g_signal_connect_swapped (G_OBJECT (fs->cancel_button), "clicked",
			    G_CALLBACK (file_dialog_hide),
			    filesel);
  g_signal_connect (G_OBJECT (filesel), "delete_event",
		    G_CALLBACK (file_dialog_hide),
		    NULL);

  g_signal_connect (G_OBJECT (fs->ok_button), "clicked",
		    G_CALLBACK (ok_callback),
		    filesel);

  /*  The file type menu  */
  {
    GtkWidget *hbox;
    GtkWidget *option_menu;
    GtkWidget *label;

    hbox = gtk_hbox_new (FALSE, 4);

    option_menu = gtk_option_menu_new ();
    gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
    gtk_widget_show (option_menu);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu),
                              GTK_ITEM_FACTORY (item_factory)->widget);

    gtk_box_pack_end (GTK_BOX (fs->main_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Determine File Type:"));
    gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
  }

  return filesel;
}

void
file_dialog_show (GtkWidget *filesel)
{
  gimp_item_factories_set_sensitive ("<Toolbox>", "/File/Open...", FALSE);

  gimp_item_factories_set_sensitive ("<Image>", "/File/Open...", FALSE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save", FALSE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save as...", FALSE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save a Copy...", FALSE);

  gtk_widget_grab_focus (GTK_FILE_SELECTION (filesel)->selection_entry);
  gtk_widget_show (filesel);
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

void
file_dialog_update_menus (GSList        *procs,
			  GimpImageType  image_type)
{
  PlugInProcDef   *file_proc;
  PlugInImageType  plug_in_image_type = 0;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
      plug_in_image_type = PLUG_IN_RGB_IMAGE;
      break;
    case GIMP_RGBA_IMAGE:
      plug_in_image_type = PLUG_IN_RGBA_IMAGE;
      break;
    case GIMP_GRAY_IMAGE:
      plug_in_image_type = PLUG_IN_GRAY_IMAGE;
      break;
    case GIMP_GRAYA_IMAGE:
      plug_in_image_type = PLUG_IN_GRAYA_IMAGE;
      break;
    case GIMP_INDEXED_IMAGE:
      plug_in_image_type = PLUG_IN_INDEXED_IMAGE;
      break;
    case GIMP_INDEXEDA_IMAGE:
      plug_in_image_type = PLUG_IN_INDEXEDA_IMAGE;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  for (; procs; procs = g_slist_next (procs))
    {
      file_proc = (PlugInProcDef *) procs->data;

      if (file_proc->db_info.proc_type != GIMP_EXTENSION)
        {
          GtkItemFactory *item_factory;

          item_factory =
            GTK_ITEM_FACTORY (gimp_item_factory_from_path (file_proc->menu_path));

          gimp_item_factory_set_sensitive (item_factory,
                                           file_proc->menu_path,
                                           (file_proc->image_types_val & plug_in_image_type));
        }
    }
}
