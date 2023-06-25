/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "script-fu-console-total.h"

#include "script-fu-intl.h"

/* TotalHistory
 * Model for the history pane of SFConsole.
 *
 * Shows welcome, prompts repr, commands executed, and results of evaluation.
 *
 * A thin wrapper around GtkTextBuffer
 *
 * All changes to the model update the view via signals of the underlying Gtk classes.
 */


GtkTextBuffer *
console_total_history_new (void)
{
  return gtk_text_buffer_new (NULL);
}

/* Clear TotalHistory.
 *
 * !!! Not clear CommandHistory, only TotalHistory.
 * So TotalHistory is not always a superset of CommandHistory.
 * FUTURE: also clear CommandHistory??
 *
 * !!! Not affect cursor of CommandHistory
 * FUTURE: reset cursor to bottom?
 */
void
console_total_history_clear (GtkTextBuffer *self)
{
  GtkTextIter start, end;

  gtk_text_buffer_get_start_iter (self, &start);
  gtk_text_buffer_get_end_iter (self, &end);
  gtk_text_buffer_delete (self, &start, &end);
}

/* Get all the text in self, including text
 * that is not in CommandHistory, i.e. splash, prompts, and results.
 */
gchar *
console_total_history_get_text (GtkTextBuffer *self)
{
  GtkTextIter start, end;

  gtk_text_buffer_get_start_iter (self, &start);
  gtk_text_buffer_get_end_iter (self, &end);
  return gtk_text_buffer_get_text (self, &start, &end, FALSE);
}

void
console_total_append_welcome (GtkTextBuffer *self)
{
  gtk_text_buffer_create_tag (self, "strong",
                              "weight", PANGO_WEIGHT_BOLD,
                              "scale",  PANGO_SCALE_LARGE,
                              NULL);
  gtk_text_buffer_create_tag (self, "emphasis",
                              "style",  PANGO_STYLE_OBLIQUE,
                              NULL);

  {
    const gchar * const greetings[] =
    {
      "emphasis", N_("Welcome to TinyScheme"),
      NULL,       "\n",
      "emphasis", "Copyright (c) Dimitrios Souflis",
      NULL,       "\n",
      "emphasis", N_("Scripting GIMP in the Scheme language"),
      NULL,       "\n"
    };

    GtkTextIter cursor;
    gint        i;

    gtk_text_buffer_get_end_iter (self, &cursor);

    for (i = 0; i < G_N_ELEMENTS (greetings); i += 2)
      {
        if (greetings[i])
          gtk_text_buffer_insert_with_tags_by_name (self, &cursor,
                                                    gettext (greetings[i + 1]),
                                                    -1, greetings[i],
                                                    NULL);
        else
          gtk_text_buffer_insert (self, &cursor,
                                  gettext (greetings[i + 1]), -1);
      }
  }

}

void
console_total_append_text_normal (GtkTextBuffer *self,
                                  const gchar  *text,
                                  gint          len)
{
  GtkTextIter    cursor;

  gtk_text_buffer_get_end_iter (self, &cursor);
  gtk_text_buffer_insert (self, &cursor, text, len);
  gtk_text_buffer_insert (self, &cursor, "\n", 1);
}

void
console_total_append_text_emphasize (GtkTextBuffer *self,
                                     const gchar   *text,
                                     gint           len)
{
  GtkTextIter    cursor;

  gtk_text_buffer_get_end_iter (self, &cursor);
  gtk_text_buffer_insert_with_tags_by_name (self,
                                            &cursor,
                                            text, len, "emphasis",
                                            NULL);
  gtk_text_buffer_insert (self, &cursor, "\n", 1);
}

/* Write newlines, prompt, and command. */
void
console_total_append_command (GtkTextBuffer *self,
                              const gchar   *command)
{
  GtkTextIter  cursor;

  gtk_text_buffer_get_end_iter (self, &cursor);

  /* assert we are just after a newline. */

  /* Write repr of a prompt.
   * SFConsole doesn't have a prompt in it's command line,
   * But we show one in the history view to distinguish commands.
   */
  gtk_text_buffer_insert_with_tags_by_name (self, &cursor,
                                            "> ", 2,
                                            "strong",
                                            NULL);
  gtk_text_buffer_insert (self, &cursor, command, -1);
  gtk_text_buffer_insert (self, &cursor, "\n", 1);
}
