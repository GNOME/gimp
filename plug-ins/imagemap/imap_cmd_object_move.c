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

#include "libgimp/stdplugins-intl.h"

static void object_move_command_destruct(Command_t *parent);
static CmdExecuteValue_t object_move_command_execute(Command_t *parent);
static void object_move_command_undo(Command_t *parent);

static CommandClass_t object_move_command_class = {
   object_move_command_destruct,
   object_move_command_execute,
   object_move_command_undo,
   NULL                         /* object_move_command_redo */
};

typedef struct {
   Command_t parent;
   Object_t *obj;
   gint dx;
   gint dy;
} ObjectMoveCommand_t;

Command_t*
object_move_command_new(Object_t *obj, gint dx, gint dy)
{
   ObjectMoveCommand_t *command = g_new(ObjectMoveCommand_t, 1);
   command->obj = object_ref(obj);
   command->dx = dx;
   command->dy = dy;
   return command_init(&command->parent, _("Move"),
                       &object_move_command_class);
}

static void
object_move_command_destruct(Command_t *parent)
{
   ObjectMoveCommand_t *command = (ObjectMoveCommand_t*) parent;
   object_unref(command->obj);
}

static CmdExecuteValue_t
object_move_command_execute(Command_t *parent)
{
   ObjectMoveCommand_t *command = (ObjectMoveCommand_t*) parent;
   object_move(command->obj, command->dx, command->dy);
   return CMD_APPEND;
}

static void
object_move_command_undo(Command_t *parent)
{
   ObjectMoveCommand_t *command = (ObjectMoveCommand_t*) parent;
   object_move(command->obj, -command->dx, -command->dy);
}
