/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * errorconsole.c - text window for collecting error messages
 * Copyright (C) 1998 Nick Fetchak <nuke@bayside.net>
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

#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef NATIVE_WIN32
#include <io.h>
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#endif

#include "actionarea.h"

#include <gtk/gtk.h>

#include "appenv.h"
#include "commands.h"
#include "session.h"
#include "dialog_handler.h"

#include "libgimp/gimpintl.h"

#define ERRORS_ALL       0
#define ERRORS_SELECTION 1

static GtkWidget * error_console = NULL;
static GtkWidget * text;


static void
error_console_close_callback (GtkWidget	*widget,
			      gpointer	 data)
{
  gtk_widget_hide (error_console);

  /* FIXME: interact with preferences */
  message_handler = MESSAGE_BOX;
}

static void
error_console_clear_callback (GtkWidget	*widget,
			      gpointer	 data)
{
  gtk_editable_delete_text
    (GTK_EDITABLE (text), 0, gtk_text_get_length (GTK_TEXT (text)));
}

static gint
error_console_delete_callback (GtkWidget *widget,
			       GdkEvent  *event,
			       gpointer	  data)
{
  error_console_close_callback (NULL, NULL);

  return TRUE;
}

void
error_console_free (void)
{                                     
  if (error_console)
    session_get_window_info (error_console, &error_console_session_info);
}

gint
error_console_write_file (gchar *path,
			  gint   textscope)
{
  gint	fd;
  gint	text_length;
  gint	bytes_written;
  gchar	*text_contents;
  GtkText	*gtext;
  
  gtext = GTK_TEXT(text);

  fd = open (path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
  
  if (fd == -1)
    return FALSE;

  if (textscope == ERRORS_ALL)
    {
      text_contents =
        gtk_editable_get_chars (GTK_EDITABLE (text), 0,
				gtk_text_get_length (GTK_TEXT (text)));
    }
  else
    {
      gint selection_start, selection_end, temp;
      
      selection_start = GTK_TEXT (text)->editable.selection_start_pos;
      selection_end = GTK_TEXT (text)->editable.selection_end_pos;

      if (selection_start > selection_end)
        {
	  temp = selection_start;
	  selection_start = selection_end;
	  selection_end = temp;
	}

      text_contents = gtk_editable_get_chars (GTK_EDITABLE (text),
					      selection_start,
					      selection_end);
    }

  text_length = strlen (text_contents);

  if (text_contents && (text_length > 0))
    {
      bytes_written = write (fd, text_contents, text_length);
      
      g_free (text_contents);
      close (fd);
      
      if (bytes_written != text_length)
        return FALSE;
      else
	return TRUE;
    }

  close (fd);
  return TRUE;
}

static void
error_console_file_ok_callback (GtkWidget *widget,
				gpointer   data)
{
  GtkWidget	*filesel;
  gchar		*filename;
  gint		textscope;

  filesel = (GtkWidget *) data;
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
  
  textscope = (gint) gtk_object_get_user_data (GTK_OBJECT (filesel));

  if (!error_console_write_file (filename, textscope))
    {
      GString	*string;

      string = g_string_new ("");
      g_string_sprintf (string, _("Error opening file %s: %s"), filename, g_strerror (errno));
      g_message (string->str);
      g_string_free (string, TRUE);
    }
  else
    gtk_widget_destroy (filesel);
}

static void
error_console_menu_callback (gint textscope)
{
  GtkWidget	*filesel;

  if (!(GTK_TEXT (text)->editable.has_selection) && (textscope == ERRORS_SELECTION))
    {
      g_message (_("Can't save, nothing selected!"));
      return;
    }
  
  filesel = gtk_file_selection_new (_("Save error log to file..."));
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (filesel), "save_errors", "Gimp");
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			     "clicked", (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (filesel));

  gtk_object_set_user_data (GTK_OBJECT (filesel), (gpointer) textscope);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
  		      "clicked", (GtkSignalFunc) error_console_file_ok_callback,
		      filesel);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			     "delete_event", (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (filesel));
  gtk_widget_show (filesel);
}

static gint
text_clicked_callback (GtkWidget        *widget,
		       GdkEventButton   *event, 
		       gpointer		 data)
{
  GtkMenu	*menu = (GtkMenu *) data;
  GtkText	*gtext;
  
  gtext = GTK_TEXT (text);

  switch (event->button)
    {
    case 1:
    case 2:
      break;

    case 3:
      gtk_signal_emit_stop_by_name (GTK_OBJECT (text), "button_press_event");
      gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event->button, event->time);

      /*  wheelmouse support  */
    case 4:
      {
	GtkAdjustment *adj = gtext->vadj;
	gfloat new_value = adj->value - adj->page_increment / 2;
	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      break;

    case 5:
      {
	GtkAdjustment *adj = gtext->vadj;
	gfloat new_value = adj->value + adj->page_increment / 2;
	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      break;

    default:
      break;
    }

  return TRUE; 
}

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { N_("Clear"), error_console_clear_callback, NULL, NULL },
  { N_("Close"), error_console_close_callback, NULL, NULL }
};

static void
error_console_create_window (void)
{
  GtkWidget	*table;
  GtkWidget	*vscrollbar;
  GtkWidget	*menu;
  GtkWidget	*menuitem;

  error_console = gtk_dialog_new ();

  /* register this one only */
  dialog_register (error_console);

  gtk_window_set_wmclass (GTK_WINDOW (error_console), "error_console", "Gimp");
  gtk_window_set_title (GTK_WINDOW (error_console), _("GIMP Error console"));
  session_set_window_geometry (error_console, &error_console_session_info, TRUE); 
  /* The next line should disappear when setting the size works in SM */
  gtk_widget_set_usize (error_console, 250, 300);
  gtk_window_set_policy (GTK_WINDOW(error_console), TRUE, TRUE, FALSE);
  gtk_signal_connect (GTK_OBJECT (error_console), "delete_event",
		      (GdkEventFunc) error_console_delete_callback, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (error_console)->vbox), 2);

  menu = gtk_menu_new ();

  menuitem = gtk_menu_item_new_with_label (_("Write all errors to file..."));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_signal_connect_object (GTK_OBJECT(menuitem), "activate",
			     (GtkSignalFunc) error_console_menu_callback,
			     (gpointer) ERRORS_ALL);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_menu_item_new_with_label (_("Write selection to file..."));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_signal_connect_object (GTK_OBJECT(menuitem), "activate",
			     (GtkSignalFunc) error_console_menu_callback,
			     (gpointer) ERRORS_SELECTION);
  gtk_widget_show (menuitem);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (error_console)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /*  The output text widget  */
  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (text), TRUE);
  gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_widget_set_events (text, GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect (GTK_OBJECT (text), "button_press_event",
		      GTK_SIGNAL_FUNC (text_clicked_callback), GTK_MENU (menu));

  gtk_widget_show (text);

  vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
  gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vscrollbar);

  /*  Action area  */
  action_items[0].user_data = error_console;
  action_items[1].user_data = text;
  build_action_area (GTK_DIALOG (error_console), action_items, 2, 0);

  gtk_widget_show (error_console);
}

void
error_console_add (gchar *errormsg)
{
  if (!error_console)
    {
      error_console_create_window ();

      /* FIMXE: interact with preferences */
      message_handler = ERROR_CONSOLE;
    } 
  else 
    {
      if (!GTK_WIDGET_VISIBLE (error_console))
	{
	  gtk_widget_show (error_console);

	  /* FIXME: interact with preferences */
	  message_handler = ERROR_CONSOLE;
	}
      else 
	gdk_window_raise (error_console->window);
    }
    
  if (errormsg)
    {
      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, errormsg, -1);
      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, "\n", -1);
    }
}
