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

#include "gui-types.h"

#include "plug-in/plug-in.h"

#include "widgets/gimpitemfactory.h"

#include "file-dialog-utils.h"


void
file_dialog_show (GtkWidget *filesel)
{
  gimp_menu_item_set_sensitive ("<Toolbox>/File/Open...", FALSE);
  gimp_menu_item_set_sensitive ("<Image>/File/Open...", FALSE);
  gimp_menu_item_set_sensitive ("<Image>/File/Save", FALSE);
  gimp_menu_item_set_sensitive ("<Image>/File/Save as...", FALSE);
  gimp_menu_item_set_sensitive ("<Image>/File/Save a Copy as...", FALSE);

  gtk_widget_grab_focus (GTK_FILE_SELECTION (filesel)->selection_entry);
  gtk_widget_show (filesel);
}

gboolean
file_dialog_hide (GtkWidget *filesel)
{
  gtk_widget_hide (filesel);
  
  gimp_menu_item_set_sensitive ("<Toolbox>/File/Open...", TRUE);
  gimp_menu_item_set_sensitive ("<Image>/File/Open...", TRUE);
  gimp_menu_item_set_sensitive ("<Image>/File/Save", TRUE);
  gimp_menu_item_set_sensitive ("<Image>/File/Save as...", TRUE);
  gimp_menu_item_set_sensitive ("<Image>/File/Save a Copy as...", TRUE);

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
          gimp_menu_item_set_sensitive (file_proc->menu_path,
                                        (file_proc->image_types_val & plug_in_image_type));
        }
    }
}
