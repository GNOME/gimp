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

#ifdef G_OS_WIN32
#include <io.h>
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#endif

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "appenv.h"

#include "libgimp/gimpintl.h"


enum
{
  ERRORS_ALL,
  ERRORS_SELECTION
};


static GtkWidget     *error_console = NULL;
static GtkWidget     *text_view     = NULL;
static GtkTextBuffer *text_buffer   = NULL;


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
  GtkTextBuffer *buffer;
  GtkTextIter    start_iter;
  GtkTextIter    end_iter;

  buffer = (GtkTextBuffer *) data;

  gtk_text_buffer_get_start_iter (buffer, &start_iter);
  gtk_text_buffer_get_end_iter (buffer, &end_iter);

  gtk_text_buffer_delete (buffer, &start_iter, &end_iter);
}

void
error_console_free (void)
{                                     
  if (error_console)
    {
      gtk_widget_destroy (error_console);
      error_console = NULL;
    }
}

static gboolean
error_console_write_file (const gchar *path,
			  gint         textscope)
{
  GtkTextIter  start_iter;
  GtkTextIter  end_iter;
  gint         fd;
  gint         text_length;
  gint         bytes_written;
  gchar	      *text_contents;

  fd = open (path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
  
  if (fd == -1)
    return FALSE;

  if (textscope == ERRORS_ALL)
    {
      gtk_text_buffer_get_bounds (text_buffer, &start_iter, &end_iter);
    }
  else
    {
      gtk_text_buffer_get_selection_bounds (text_buffer, &start_iter, &end_iter);
    }

  text_contents = gtk_text_buffer_get_text (text_buffer,
					    &start_iter, &end_iter, TRUE);

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
  GtkWidget   *filesel;
  const gchar *filename;
  gint	       textscope;

  filesel  = (GtkWidget *) data;
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));

  textscope = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (filesel),
						  "text-scope"));

  if (! error_console_write_file (filename, textscope))
    {
      gchar *message;

      message = g_strdup_printf (_("Error opening file %s: %s"),
				 filename, g_strerror (errno));

      g_message (message);

      g_free (message);
    }
  else
    {
      gtk_widget_destroy (filesel);
    }
}

static void
error_console_menu_callback (gint textscope)
{
  GtkWidget *filesel;

  if (! gtk_text_buffer_get_selection_bounds (text_buffer, NULL, NULL) &&
      textscope == ERRORS_SELECTION)
    {
      g_message (_("Can't save, nothing selected!"));
      return;
    }

  filesel = gtk_file_selection_new (_("Save error log to file..."));
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (filesel), "save_errors", "Gimp");

  gtk_container_set_border_width (GTK_CONTAINER (filesel), 2);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_FILE_SELECTION (filesel)->button_area), 2);

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			    "clicked",
			    G_CALLBACK (gtk_widget_destroy),
			    filesel);

  g_object_set_data (G_OBJECT (filesel), "text-scope",
		     GINT_TO_POINTER (textscope));
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		    "clicked",
		    G_CALLBACK (error_console_file_ok_callback),
		    filesel);

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			    "delete_event",
			    G_CALLBACK (gtk_widget_destroy),
			    filesel);

  /*  Connect the "F1" help key  */
  gimp_help_connect (filesel,
		     gimp_standard_help_func,
		     "dialogs/error_console.html");

  gtk_widget_show (filesel);
}

static gboolean
text_clicked_callback (GtkWidget      *widget,
		       GdkEventButton *event,
		       gpointer	       data)
{
  GtkMenu *menu = (GtkMenu *) data;
  
  menu  = (GtkMenu *) data;

  switch (event->button)
    {
    case 1:
    case 2:
      return FALSE;
      break;

    case 3:
      gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event->button, event->time);

      /*  wheelmouse support  */
    case 4:
      {
	GtkAdjustment *adj = GTK_TEXT_VIEW (text_view)->vadjustment;
	gdouble new_value = adj->value - adj->page_increment / 2;
	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      break;

    case 5:
      {
	GtkAdjustment *adj = GTK_TEXT_VIEW (text_view)->vadjustment;
	gdouble new_value = adj->value + adj->page_increment / 2;
	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      break;

    default:
      break;
    }

  return TRUE;
}

GtkWidget *
error_console_create (void)
{
  GtkWidget *scrolled_window;
  GtkWidget *menu;
  GtkWidget *menuitem;

  if (error_console)
    return error_console;

  text_buffer = gtk_text_buffer_new  (NULL);

  error_console = gimp_dialog_new (_("GIMP Error Console"), "error_console",
				   gimp_standard_help_func,
				   "dialogs/error_console.html",
				   GTK_WIN_POS_NONE,
				   TRUE, TRUE, FALSE,

				   _("Clear"), error_console_clear_callback,
				   text_buffer, NULL, NULL, FALSE, FALSE,
				   GTK_STOCK_CLOSE, error_console_close_callback,
				   text_buffer, NULL, NULL, TRUE, TRUE,

				   NULL);

  /* The next line should disappear when setting the size works in SM */
  gtk_widget_set_usize (error_console, 250, 300);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (error_console)->vbox),
		     scrolled_window);
  gtk_widget_show (scrolled_window);

  menu = gtk_menu_new ();

  menuitem = gtk_menu_item_new_with_label (_("Write all errors to file..."));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect_swapped (G_OBJECT (menuitem), "activate",
			    G_CALLBACK (error_console_menu_callback),
			    GINT_TO_POINTER (ERRORS_ALL));
  
  menuitem = gtk_menu_item_new_with_label (_("Write selection to file..."));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect_swapped (G_OBJECT (menuitem), "activate",
			   G_CALLBACK (error_console_menu_callback),
			   GINT_TO_POINTER (ERRORS_SELECTION));

  /*  The output text widget  */
  text_view = gtk_text_view_new_with_buffer (text_buffer);
  g_object_unref (G_OBJECT (text_buffer));

  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  g_signal_connect (G_OBJECT (text_view), "button_press_event",
		    G_CALLBACK (text_clicked_callback),
		    GTK_MENU (menu));

  gtk_widget_show (error_console);

  /* FIXME: interact with preferences */
  message_handler = ERROR_CONSOLE;

  return error_console;
}

void
error_console_add (const gchar *errormsg)
{
  if (! error_console)
    {
      error_console_create ();
    }
  else 
    {
      if (! GTK_WIDGET_VISIBLE (error_console))
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
      GtkTextIter end;

      gtk_text_buffer_get_end_iter (text_buffer, &end);
      gtk_text_buffer_place_cursor (text_buffer, &end);

      gtk_text_buffer_insert_at_cursor (text_buffer, errormsg, -1);
      gtk_text_buffer_insert_at_cursor (text_buffer, "\n", -1);
    }
}
