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

#include "libgimp/gimp.h"

#include "script-fu-console-history.h"

static gint            console_history_tail_position (CommandHistory *self);
static GStrv           console_history_to_strv       (CommandHistory *self);


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
 * CommandHistory is persistent across sessions of the ScriptFu Console,
 * and across sessions of Gimp.
 * When the SFConsole starts, the TotalHistory,
 * is just the CommandHistory, without results of eval.
 * Old results are not meaningful since the environment changed.
 * Specifically, a new session of SFConsole has a new initialized interpreter.
 * Similarly, when the user closes the console,
 * only the CommandHistory is saved as settings.
 */

void
console_history_init (CommandHistory *self)
{
  self->model     = g_list_append (self->model, NULL);
  self->model_len = 1;
  self->model_max = 100;
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
                     console_history_tail_position (self));

  if (list->data)
    g_free (list->data);

  /* Discarding const. */
  list->data = (gpointer) command;
}

/* Remove the head of the history and free its string.
 *
 * GList doesn't have such a direct method.
 * Search web to find this solution.
 * !!! g_list_remove does not free the data of the removed element.
 *
 * Remove the element whose data (a string)
 * matches the data of the first element.
 * Then free the data of the first element.
 */
static void
console_history_remove_head (CommandHistory *self)
{
  gpointer * data;

  g_return_if_fail (self->model != NULL);

  data = self->model->data;
  self->model = g_list_remove (self->model, data);
  g_free (data);
}

/* Append NULL string at tail of CommandHistory.
 * Prune head when max exceeded, freeing the string.
 * Position the cursor at last element.
 */
void
console_history_new_tail (CommandHistory *self)
{
  self->model = g_list_append (self->model, NULL);

  if (self->model_len == self->model_max)
    {
      console_history_remove_head (self);
    }
  else
    {
      self->model_len++;
    }

  self->model_cursor = console_history_tail_position (self);
}

void
console_history_cursor_to_tail (CommandHistory *self)
{
  self->model_cursor = console_history_tail_position (self);
}

gboolean
console_history_is_cursor_at_tail (CommandHistory *self)
{
  return self->model_cursor == console_history_tail_position (self);
}

void
console_history_move_cursor (CommandHistory *self,
                             gint            direction)
{
  self->model_cursor += direction;

  /* Clamp cursor in range [0, model_len-1] */
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


/* Methods for persisting history as a setting. */

/* Return a GStrv of the history from settings.
 * The Console knows how to put GStrv to both models!
 *
 * !!! Handle attack on settings file.
 * The returned cardinality of the set of strings
 * may be zero or very many.
 * Elsewhere ensure we don't overflow models.
 */
GStrv
console_history_from_settings (CommandHistory       *self,
                               GimpProcedureConfig  *config)
{
  GStrv           in_history;

  /* Get aux arg from property of config. */
  g_object_get (config,
                "history", &in_history,
                NULL);

  return in_history;
}

void
console_history_to_settings (CommandHistory       *self,
                             GimpProcedureConfig  *config)
{
  GStrv           out_history;

  out_history = console_history_to_strv (self);

  /* set an aux arg in config. */
  g_object_set (config,
                "history", out_history,
                NULL);
}

/* Return history model as GStrv.
 * Converts from interal list into a string array.
 *
 * !!! The exported history may have a tail
 * which is user's edits to the command line,
 * that the user never evaluated.
 * Exported history does not have an empty tail.
 *
 * Caller must g_strfreev the returned GStrv.
 */
static GStrv
console_history_to_strv (CommandHistory *self)
{
  GStrv         history_strv;
  GStrvBuilder *builder;

  builder = g_strv_builder_new ();
  /* Order is earliest first. */
  for (GList *l = self->model; l != NULL; l = l->next)
    {
      /* Don't write an empty pre-allocated tail. */
      if (l->data != NULL)
        g_strv_builder_add (builder, l->data);
    }
  history_strv = g_strv_builder_end (builder);
  g_strv_builder_unref (builder);
  return history_strv;
}

static gint
console_history_tail_position (CommandHistory *self)
{
  return g_list_length (self->model) - 1;
}