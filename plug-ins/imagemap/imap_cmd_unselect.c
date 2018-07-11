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

static void unselect_command_destruct(Command_t *parent);
static CmdExecuteValue_t unselect_command_execute(Command_t *parent);
static void unselect_command_undo(Command_t *parent);

static CommandClass_t unselect_command_class = {
   unselect_command_destruct,
   unselect_command_execute,
   unselect_command_undo,
   NULL                         /* unselect_command_redo */
};

typedef struct {
   Command_t parent;
   Object_t *obj;
} UnselectCommand_t;

Command_t*
unselect_command_new(Object_t *obj)
{
   UnselectCommand_t *command = g_new(UnselectCommand_t, 1);
   command->obj = object_ref(obj);
   return command_init(&command->parent, _("Unselect"),
                       &unselect_command_class);
}

static void
unselect_command_destruct(Command_t *command)
{
   UnselectCommand_t *unselect_command = (UnselectCommand_t*) command;
   object_unref(unselect_command->obj);
}

static CmdExecuteValue_t
unselect_command_execute(Command_t *command)
{
   UnselectCommand_t *unselect_command = (UnselectCommand_t*) command;
   object_unselect(unselect_command->obj);
   return CMD_APPEND;
}

static void
unselect_command_undo(Command_t *command)
{
   UnselectCommand_t *unselect_command = (UnselectCommand_t*) command;
   object_select(unselect_command->obj);
}
