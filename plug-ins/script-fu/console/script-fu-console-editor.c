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

#include "script-fu-console-editor.h"


/* ConsoleEditor

 * An API that abstracts these GtkWidgets for editing text:
 * - GtkTextEntry is single line,
 * - GtkTextView is multiline.
 *
 * So that we can swap out or enhance widget without affecting main logic.
 *
 * Not a defined class but methods conform to naming and calling conventions.
 *
 * Is-a GtkWidget.
 */

/* FUTURE
 * GtkTextEntry => GtkTextView (multiline)
 *
 * Possibly: an editor that understands syntax and highlighting.
 */

GtkWidget *
console_editor_new (void)
{
  return gtk_entry_new ();
}

/* Set editor's text and position the cursor.
 * @position conforms to GtkEditable interface.
 */
void
console_editor_set_text_and_position (GtkWidget   *self,
                                      const gchar *text,
                                      gint         position)
{
  /* gtk_entry_set_text not allow NULL */
  if (text != NULL)
    gtk_entry_set_text (GTK_ENTRY (self), text);
  gtk_editable_set_position (GTK_EDITABLE (self), position);
}

void
console_editor_clear (GtkWidget *self)
{
  console_editor_set_text_and_position (self, "", -1);
}

const gchar *
console_editor_get_text (GtkWidget *self)
{
  return gtk_entry_get_text (GTK_ENTRY (self));
}

gboolean
console_editor_is_empty (GtkWidget *self)
{
  const gchar *str;

  if ((str = console_editor_get_text (self)) == NULL)
    return TRUE;

  while (*str)
    {
      if (*str != ' ' && *str != '\t' && *str != '\n')
        return FALSE;

      str ++;
    }

  return TRUE;
}
