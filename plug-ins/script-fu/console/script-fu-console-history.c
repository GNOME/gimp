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

#include "script-fu-console-history.h"


/* CommandHistory
 *
 * Not a true class, just a struct with methods.
 * Does not inherit GObject.
 *
 * Is a model that affects a view of the "console history."
 * The model is a sequence of ExecutedCommands.
 * The sequence is a time-ordered queue.
 * You can only append to the end, called the tail.
 * An ExecutedCommand does not contain the result of interpretation,
 * only the string that was interpreted.
 *
 * The sequence is finite.
 * When you append to the tail,
 * commands might be discarded from the head.
 *
 * Has a cursor.
 * You can only get the command at the cursor.
 * The user scrolling through the history moves the cursor.
 * This scrolling is arrow keys in the editor pane,
 * (not scroll bars in the history view pane.)
 * See the main console logic:
 * when user hits arrow keys in the editor,
 * move cursor in the history, get the command at the cursor,
 * and display it in the editor, ready to execute.
 *
 * A CommandHistory is a model,
 * but there is also a distinct TotalHistory model for a scrolling view of the history
 * (e.g. a GtkTextBuffer model for a GtkTextView.)
 *
 * CommandHistory <-supersets- TotalHistory <-views- ConsoleView
 *
 * TotalHistory contains more than the commands in CommandHistory.
 * TotalHistory contains e.g. splash, prompts, and interpretation results.
 *
 * !!! Self does not currently write TotalHistory;
 * The main console logic writes TotalHistory,
 *
 * FUTURE:
 * CommandHistory is persistent across sessions of the ScriptFu Console,
 * and across sessions of Gimp.
 * When the SFConsole starts, the TotalHistory,
 * is just the CommandHistory, without results.
 * Old results are not meaningful since the environment changed.
 * Specifically, a new session of SFConsole has a new initialized interpreter.
 * Similarly, when the user quits the console,
 * only the CommandHistory is saved as settings.
 */

void
console_history_init (CommandHistory *self,
                      GtkTextBuffer  *model_of_view)
{
  self->model     = g_list_append (self->model, NULL);
  self->model_len = 1;
  self->model_max = 50;
}



/* Store the command in tail of CommandHistory.
 * The tail is the most recent added element, which was created prior by new_tail.
 *
 * @commmand transfer full
 *
 * !!! The caller is executing the command.
 * The caller updates TotalHistory, with a prompt string and the command string.
 * Self does not update TotalHistory, the model of the view.
 */
void
console_history_set_tail (CommandHistory *self,
                          const gchar    *command)
{
  GList *list;

  list = g_list_nth (self->model,
                     (g_list_length (self->model) - 1));

  if (list->data)
    g_free (list->data);

  /* Discarding const. */
  list->data = (gpointer) command;
}

/* Allocate an empty element at tail of CommandHistory.
 * Prune head when max exceeded.
 * Position the cursor at last element.
 */
void
console_history_new_tail (CommandHistory *self)
{
  self->model = g_list_append (self->model, NULL);

  if (self->model_len == self->model_max)
    {
      /* FIXME: is this correct, seems to be double freeing the head. */
      self->model = g_list_remove (self->model, self->model->data);
      if (self->model->data)
        g_free (self->model->data);
    }
  else
    {
      self->model_len++;
    }

  self->model_cursor = g_list_length (self->model) - 1;
}

gboolean
console_history_is_cursor_at_tail (CommandHistory *self)
{
  return self->model_cursor == g_list_length (self->model) - 1;
}

void
console_history_move_cursor (CommandHistory *self,
                             gint            direction)
{
  self->model_cursor += direction;

  if (self->model_cursor < 0)
    self->model_cursor = 0;

  if (self->model_cursor >= self->model_len)
    self->model_cursor = self->model_len - 1;
}

const gchar *
console_history_get_at_cursor (CommandHistory *self)
{
  return g_list_nth (self->model, self->model_cursor)->data;
}