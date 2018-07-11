/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2003 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimp/gimp.h"

#include "imap_commands.h"
#include "imap_main.h"

#include "libgimp/stdplugins-intl.h"

static void move_command_destruct(Command_t *parent);
static CmdExecuteValue_t move_command_execute(Command_t *parent);

static CommandClass_t move_command_class = {
   move_command_destruct,
   move_command_execute,
   NULL,                        /* move_command_undo */
   NULL                         /* move_command_redo */
};

typedef struct {
   Command_t parent;
   PreferencesData_t *preferences;
   Preview_t *preview;
   Object_t *obj;
   gint start_x;
   gint start_y;
   gint obj_start_x;
   gint obj_start_y;
   gint obj_x;
   gint obj_y;
   gint obj_width;
   gint obj_height;

   gint image_width;
   gint image_height;

   GdkCursorType cursor;        /* Remember previous cursor */
   gboolean moved_first_time;
} MoveCommand_t;

Command_t*
move_command_new(Preview_t *preview, Object_t *obj, gint x, gint y)
{
   MoveCommand_t *command = g_new(MoveCommand_t, 1);

   command->preferences = get_preferences();
   command->preview = preview;
   command->obj = object_ref(obj);
   command->start_x = x;
   command->start_y = y;
   object_get_dimensions(obj, &command->obj_x, &command->obj_y,
                         &command->obj_width, &command->obj_height);
   command->obj_start_x = command->obj_x;
   command->obj_start_y = command->obj_y;

   command->image_width = get_image_width();
   command->image_height = get_image_height();

   command->moved_first_time = TRUE;

   return command_init(&command->parent, _("Move"), &move_command_class);
}

static void
move_command_destruct(Command_t *parent)
{
   MoveCommand_t *command = (MoveCommand_t*) parent;
   object_unref(command->obj);
}

static void
button_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
   MoveCommand_t *command = (MoveCommand_t*) data;
   Object_t *obj = command->obj;
   gint dx = get_real_coord((gint) event->x) - command->start_x;
   gint dy = get_real_coord((gint) event->y) - command->start_y;

   if (command->moved_first_time) {
      command->moved_first_time = FALSE;
      command->cursor = preview_set_cursor(command->preview, GDK_FLEUR);
      hide_url();
   }

   if (command->obj_x + dx < 0)
      dx = -command->obj_x;
   if (command->obj_x + command->obj_width + dx > command->image_width)
      dx = command->image_width - command->obj_width - command->obj_x;
   if (command->obj_y + dy < 0)
      dy = -command->obj_y;
   if (command->obj_y + command->obj_height + dy > command->image_height)
      dy = command->image_height - command->obj_height - command->obj_y;

   if (dx || dy) {

      command->start_x = get_real_coord((gint) event->x);
      command->start_y = get_real_coord((gint) event->y);
      command->obj_x += dx;
      command->obj_y += dy;

      object_move(obj, dx, dy);

      preview_redraw ();
   }
}

static void
button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   MoveCommand_t *command = (MoveCommand_t*) data;

   g_signal_handlers_disconnect_by_func (widget,
                                         button_motion, data);
   g_signal_handlers_disconnect_by_func (widget,
                                         button_release, data);

   if (!command->moved_first_time) {
      preview_set_cursor(command->preview, command->cursor);
      show_url();
   }
   command->obj_x -= command->obj_start_x;
   command->obj_y -= command->obj_start_y;
   if (command->obj_x || command->obj_y)
      command_list_add(object_move_command_new(command->obj, command->obj_x,
                                               command->obj_y));

   /*   preview_thaw(); */
}

static CmdExecuteValue_t
move_command_execute(Command_t *parent)
{
   MoveCommand_t *command = (MoveCommand_t*) parent;
   GtkWidget *widget = command->preview->preview;

   /*   preview_freeze(); */
   g_signal_connect(widget, "button-release-event",
                    G_CALLBACK (button_release), command);
   g_signal_connect(widget, "motion-notify-event",
                    G_CALLBACK (button_motion), command);
   return CMD_DESTRUCT;
}
