/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextEditor
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets/gimptexteditor.h"

#include "gimp-intl.h"


typedef struct _TextEditorData TextEditorData;

struct _TextEditorData
{
  GtkTextBuffer *buffer;
  GtkWidget     *editor;
  GtkWidget     *filesel;
};


static void      gimp_text_editor_load      (GtkWidget      *widget,
					     TextEditorData *data);
static void      gimp_text_editor_load_ok   (TextEditorData *data);
static gboolean  gimp_text_editor_load_file (GtkTextBuffer  *buffer,
					     const gchar    *filename);
static void      gimp_text_editor_clear     (GtkWidget      *widget,
					     TextEditorData *data);


GtkWidget *
gimp_text_editor_new (const gchar   *title,
                      GtkTextBuffer *buffer)
{
  GtkWidget      *editor;
  GtkWidget      *toolbar;
  GtkWidget      *scrolled_window;
  GtkWidget      *text_view;
  TextEditorData *data;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (GTK_IS_TEXT_BUFFER (buffer), NULL);

  editor = gimp_dialog_new (title, "text_editor",
			    gimp_standard_help_func,
			    "dialogs/text_editor.html",
			    GTK_WIN_POS_NONE,
			    FALSE, TRUE, FALSE,
			    
			    GTK_STOCK_CLOSE, gtk_widget_destroy,
			    NULL, 1, NULL, TRUE, TRUE,

			    NULL);

  data = g_new0 (TextEditorData, 1);

  data->buffer = buffer;
  data->editor = editor;

  g_object_weak_ref (G_OBJECT (editor), (GWeakNotify) g_free, data);

  gtk_dialog_set_has_separator (GTK_DIALOG (editor), FALSE);

  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (editor)->vbox),
		      toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);
 
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_OPEN,
                            _("Load Text from File"), NULL,
                            G_CALLBACK (gimp_text_editor_load), data,
                            0);
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_CLEAR,
                            _("Clear all Text"), NULL,
                            G_CALLBACK (gimp_text_editor_clear), data,
                            1);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (editor)->vbox),
                      scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  text_view = gtk_text_view_new_with_buffer (buffer);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  return editor;
}

static void
gimp_text_editor_load (GtkWidget      *widget,
                       TextEditorData *data)
{
  GtkFileSelection *filesel;

  if (data->filesel)
    {
      gtk_window_present (GTK_WINDOW (data->filesel));
      return;
    }

  filesel =
    GTK_FILE_SELECTION (gtk_file_selection_new (_("Open Text File (UTF-8)")));

  gtk_window_set_wmclass (GTK_WINDOW (filesel), "gimp-text-load-file", "Gimp");
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (filesel), 2);
  gtk_container_set_border_width (GTK_CONTAINER (filesel->button_area), 2);

  gtk_window_set_transient_for (GTK_WINDOW (filesel),
                                GTK_WINDOW (data->editor));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (filesel), TRUE);

  g_signal_connect_swapped (filesel->ok_button, "clicked",
                            G_CALLBACK (gimp_text_editor_load_ok),
                            data);
  g_signal_connect_swapped (filesel->cancel_button, "clicked",
                            G_CALLBACK (gtk_widget_destroy),
                            filesel);

  data->filesel = GTK_WIDGET (filesel);
  g_object_add_weak_pointer (G_OBJECT (filesel), (gpointer) &data->filesel);

  gtk_widget_show (GTK_WIDGET (filesel));
}

static void
gimp_text_editor_load_ok (TextEditorData *data)
{
  const gchar *filename;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (data->filesel));
  
  if (gimp_text_editor_load_file (data->buffer, filename))
    gtk_widget_destroy (data->filesel);
}

static gboolean
gimp_text_editor_load_file (GtkTextBuffer *buffer,
			    const gchar   *filename)
{
  FILE        *file;
  gchar        buf[2048];
  gint         remaining = 0;
  GtkTextIter  iter;

  file = fopen (filename, "r");
  
  if (!file)
    {
      g_message (_("Error opening file '%s': %s"),
                 filename, g_strerror (errno));
      return FALSE;
    }

  gtk_text_buffer_set_text (buffer, "", 0);
  gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  while (!feof (file))
    {
      const char *leftover;
      gint count;
      gint to_read = sizeof (buf) - remaining - 1;

      count = fread (buf + remaining, 1, to_read, file);
      buf[count + remaining] = '\0';

      g_utf8_validate (buf, count + remaining, &leftover);

      gtk_text_buffer_insert (buffer, &iter, buf, leftover - buf);

      remaining = (buf + remaining + count) - leftover;
      g_memmove (buf, leftover, remaining);

      if (remaining > 6 || count < to_read)
        break;
    }

  if (remaining)
    g_message (_("Invalid UTF-8 data in file '%s'."), filename);

  return TRUE;
}

static void
gimp_text_editor_clear (GtkWidget      *widget,
                        TextEditorData *data)
{
  gtk_text_buffer_set_text (data->buffer, "", 0);
}
