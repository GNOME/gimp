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

#include "imap_commands.h"
#include "imap_main.h"

#include "libgimp/stdplugins-intl.h"

static void move_sash_command_destruct(Command_t *command);
static CmdExecuteValue_t move_sash_command_execute(Command_t *command);
static void move_sash_command_redo(Command_t *command);

static CommandClass_t move_sash_command_class = {
   move_sash_command_destruct,
   move_sash_command_execute,
   NULL /*undo*/,
   move_sash_command_redo
};

typedef struct {
   Command_t    parent;
   GtkWidget   *widget;
   Object_t    *obj;
   gint         x;
   gint         y;
   gint         image_width;
   gint         image_height;
   MoveSashFunc_t sash_func;
} MoveSashCommand_t;

Command_t*
move_sash_command_new(GtkWidget *widget, Object_t *obj,
                      gint x, gint y, MoveSashFunc_t sash_func)
{
   MoveSashCommand_t *command = g_new(MoveSashCommand_t, 1);
   Command_t *parent;

   command->widget = widget;
   command->obj = object_ref(obj);
   command->x = x;
   command->y = y;
   command->image_width = get_image_width();
   command->image_height = get_image_height();
   command->sash_func = sash_func;

   parent = command_init(&command->parent, _("Move Sash"),
                         &move_sash_command_class);
   command_add_subcommand(parent, edit_object_command_new(obj));

   return parent;
}

static void
move_sash_command_destruct(Command_t *parent)
{
   MoveSashCommand_t *command = (MoveSashCommand_t*) parent;
   object_unref(command->obj);
}

static void
sash_move(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
   MoveSashCommand_t *command = (MoveSashCommand_t*) data;
   Object_t *obj = command->obj;
   gint x, y, dx, dy;

   x = get_real_coord((gint) event->x);
   y = get_real_coord((gint) event->y);

   if (x < 0)
      x = 0;
   if (x > command->image_width)
      x = command->image_width;

   if (y < 0)
      y = 0;
   if (y > command->image_height)
      y = command->image_height;

   dx = x - command->x;
   dy = y - command->y;

   command->x = x;
   command->y = y;

   command->sash_func(obj, dx, dy);
   object_emit_geometry_signal(obj);

   preview_redraw ();
}

static void
sash_end(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   MoveSashCommand_t *command = (MoveSashCommand_t*) data;
   Object_t *obj = command->obj;

   g_signal_handlers_disconnect_by_func(widget,
                                        sash_move, data);
   g_signal_handlers_disconnect_by_func(widget,
                                        sash_end, data);
   if (obj->class->normalize)
      object_normalize(obj);
   preview_unset_tmp_obj(command->obj);
   preview_redraw();
   show_url();
}

static CmdExecuteValue_t
move_sash_command_execute(Command_t *parent)
{
   MoveSashCommand_t *command = (MoveSashCommand_t*) parent;

   hide_url();
   g_signal_connect(command->widget, "button-release-event",
                    G_CALLBACK (sash_end), command);
   g_signal_connect(command->widget, "motion-notify-event",
                    G_CALLBACK (sash_move), command);
   preview_set_tmp_obj(command->obj);

   return CMD_APPEND;
}

static void move_sash_command_redo(Command_t *command)
{
   /* do nothing, but avoid running execute again which will break event handling */
}
