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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpmarshal.h"

#include "gimpenummenu.h"
#include "gimphelp-ids.h"
#include "gimptexteditor.h"

#include "gimp-intl.h"


enum
{
  DIR_CHANGED,
  LAST_SIGNAL
};


static void      gimp_text_editor_class_init    (GimpTextEditorClass *klass);
static void      gimp_text_editor_init          (GimpTextEditor      *editor);
static void      gimp_text_editor_dispose       (GObject             *object);

static void      gimp_text_editor_dir_changed   (GtkWidget      *widget,
                                                 GimpTextEditor *editor);

static void      gimp_text_editor_load          (GtkWidget      *widget,
                                                 GimpTextEditor *editor);
static void      gimp_text_editor_load_response (GtkWidget      *dialog,
                                                 gint            response_id,
                                                 GimpTextEditor *editor);
static gboolean  gimp_text_editor_load_file     (GtkTextBuffer  *buffer,
                                                 const gchar    *filename);
static void      gimp_text_editor_clear         (GtkWidget      *widget,
                                                 GimpTextEditor *editor);


static GimpDialogClass *parent_class = NULL;
static guint            text_editor_signals[LAST_SIGNAL] = { 0 };


GType
gimp_text_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpTextEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_text_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpTextEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_text_editor_init,
      };

      type = g_type_register_static (GIMP_TYPE_DIALOG,
                                     "GimpTextEditor",
                                     &info, 0);
    }

  return type;
}

static void
gimp_text_editor_class_init (GimpTextEditorClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  text_editor_signals[DIR_CHANGED] =
    g_signal_new ("dir_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpTextEditorClass, dir_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->dispose = gimp_text_editor_dispose;

  klass->dir_changed    = NULL;
}

static void
gimp_text_editor_init (GimpTextEditor  *editor)
{
  editor->buffer  = NULL;
  editor->view    = NULL;
  editor->group   = NULL;
  editor->filesel = NULL;

  switch (gtk_widget_get_default_direction ())
    {
    case GTK_TEXT_DIR_NONE:
    case GTK_TEXT_DIR_LTR:
      editor->base_dir = GIMP_TEXT_DIRECTION_LTR;
      break;
    case GTK_TEXT_DIR_RTL:
      editor->base_dir = GIMP_TEXT_DIRECTION_RTL;
      break;
    }
}

static void
gimp_text_editor_dispose (GObject *object)
{
  GimpTextEditor *editor = GIMP_TEXT_EDITOR (object);

  if (editor->buffer)
    {
      g_object_unref (editor->buffer);
      editor->buffer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GtkWidget *
gimp_text_editor_new (const gchar   *title,
                      GtkTextBuffer *buffer)
{
  GimpTextEditor *editor;
  GtkToolbar     *toolbar;
  GtkWidget      *scrolled_window;
  GtkWidget      *box;
  GtkWidget      *button;
  GList          *children;
  GList          *list;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (GTK_IS_TEXT_BUFFER (buffer), NULL);

  editor = g_object_new (GIMP_TYPE_TEXT_EDITOR,
                         "title", title,
                         "role",  "gimp-text-editor",
                         NULL);

  gimp_help_connect (GTK_WIDGET (editor), gimp_standard_help_func,
                     GIMP_HELP_TEXT_EDITOR_DIALOG, NULL);

  gtk_dialog_add_button (GTK_DIALOG (editor),
                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  g_signal_connect (editor, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  toolbar = GTK_TOOLBAR (gtk_toolbar_new ());
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (editor)->vbox),
		      GTK_WIDGET (toolbar), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (toolbar));

  gtk_toolbar_insert_stock (toolbar, GTK_STOCK_OPEN,
                            _("Load Text from File"), NULL,
                            G_CALLBACK (gimp_text_editor_load), editor,
                            0);
  gtk_toolbar_insert_stock (toolbar, GTK_STOCK_CLEAR,
                            _("Clear all Text"), NULL,
                            G_CALLBACK (gimp_text_editor_clear), editor,
                            1);

  gtk_toolbar_append_space (toolbar);

  box = gimp_enum_stock_box_new (GIMP_TYPE_TEXT_DIRECTION,
                                 "gimp-text-dir",
                                 gtk_toolbar_get_icon_size (toolbar),
                                 G_CALLBACK (gimp_text_editor_dir_changed),
                                 editor,
                                 &editor->group);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (editor->group),
                                   editor->base_dir);

  children = gtk_container_get_children (GTK_CONTAINER (box));

  for (list = children; list; list = g_list_next (list))
    {
      button = GTK_WIDGET (list->data);

      g_object_ref (button);

      gtk_container_remove (GTK_CONTAINER (box), button);

      gtk_toolbar_append_widget (toolbar, button, NULL, NULL);

      g_object_unref (button);
    }

  g_list_free (children);
  gtk_object_sink (GTK_OBJECT (box));

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (editor)->vbox),
                      scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  editor->buffer = g_object_ref (buffer);
  editor->view = gtk_text_view_new_with_buffer (buffer);
  gtk_container_add (GTK_CONTAINER (scrolled_window), editor->view);
  gtk_widget_show (editor->view);

  switch (editor->base_dir)
    {
    case GIMP_TEXT_DIRECTION_LTR:
      gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_LTR);
      break;
    case GIMP_TEXT_DIRECTION_RTL:
      gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_RTL);
      break;
    }

  gtk_widget_set_size_request (editor->view, 128, 64);

  return GTK_WIDGET (editor);
}

void
gimp_text_editor_set_direction (GimpTextEditor    *editor,
                                GimpTextDirection  base_dir)
{
  g_return_if_fail (GIMP_IS_TEXT_EDITOR (editor));

  if (editor->base_dir == base_dir)
    return;

  editor->base_dir = base_dir;

  g_signal_handlers_block_by_func (editor->group,
                                   G_CALLBACK (gimp_text_editor_dir_changed),
                                   editor);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (editor->group), base_dir);

  g_signal_handlers_unblock_by_func (editor->group,
                                     G_CALLBACK (gimp_text_editor_dir_changed),
                                     editor);

  switch (editor->base_dir)
    {
    case GIMP_TEXT_DIRECTION_LTR:
      gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_LTR);
      break;
    case GIMP_TEXT_DIRECTION_RTL:
      gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_RTL);
      break;
    }

  g_signal_emit (editor, text_editor_signals[DIR_CHANGED], 0);
}

static void
gimp_text_editor_dir_changed (GtkWidget      *widget,
                              GimpTextEditor *editor)
{
  GimpTextDirection  dir;

  dir = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));

  gimp_text_editor_set_direction (editor, dir);
}

static void
gimp_text_editor_load (GtkWidget      *widget,
                       GimpTextEditor *editor)
{
  GtkFileSelection *filesel;

  if (editor->filesel)
    {
      gtk_window_present (GTK_WINDOW (editor->filesel));
      return;
    }

  filesel =
    GTK_FILE_SELECTION (gtk_file_selection_new (_("Open Text File (UTF-8)")));

  gtk_window_set_role (GTK_WINDOW (filesel), "gimp-text-load-file");
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (filesel), 6);
  gtk_container_set_border_width (GTK_CONTAINER (filesel->button_area), 4);

  gtk_window_set_transient_for (GTK_WINDOW (filesel), GTK_WINDOW (editor));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (filesel), TRUE);

  g_signal_connect (filesel, "response",
                    G_CALLBACK (gimp_text_editor_load_response),
                    editor);

  editor->filesel = GTK_WIDGET (filesel);
  g_object_add_weak_pointer (G_OBJECT (filesel), (gpointer) &editor->filesel);

  gtk_widget_show (GTK_WIDGET (filesel));
}

static void
gimp_text_editor_load_response (GtkWidget      *dialog,
                                gint            response_id,
                                GimpTextEditor *editor)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;

      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

      if (! gimp_text_editor_load_file (editor->buffer, filename))
        return;
    }

  gtk_widget_destroy (dialog);
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
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
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
    g_message (_("Invalid UTF-8 data in file '%s'."),
	       gimp_filename_to_utf8 (filename));

  return TRUE;
}

static void
gimp_text_editor_clear (GtkWidget      *widget,
                        GimpTextEditor *editor)
{
  gtk_text_buffer_set_text (editor->buffer, "", 0);
}
