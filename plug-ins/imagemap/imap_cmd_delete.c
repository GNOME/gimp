/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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

static void delete_command_destruct(Command_t *parent);
static CmdExecuteValue_t delete_command_execute(Command_t *parent);
static void delete_command_undo(Command_t *parent);

static CommandClass_t delete_command_class = {
   delete_command_destruct,
   delete_command_execute,
   delete_command_undo,
   NULL                         /* delete_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
   Object_t     *obj;
   gint          position;
   gboolean      changed;
} DeleteCommand_t;

Command_t*
delete_command_new(ObjectList_t *list, Object_t *obj)
{
   DeleteCommand_t *command = g_new(DeleteCommand_t, 1);
   command->list = list;
   command->obj = object_ref(obj);
   return command_init(&command->parent, _("Delete"),
                       &delete_command_class);
}

static void
delete_command_destruct(Command_t *parent)
{
   DeleteCommand_t *command = (DeleteCommand_t*) parent;
   object_unref(command->obj);
}

static CmdExecuteValue_t
delete_command_execute(Command_t *parent)
{
   DeleteCommand_t *command = (DeleteCommand_t*) parent;
   command->changed = object_list_get_changed(command->list);
   command->position = object_get_position_in_list(command->obj);
   object_list_remove(command->list, command->obj);
   return CMD_APPEND;
}

static void
delete_command_undo(Command_t *parent)
{
   DeleteCommand_t *command = (DeleteCommand_t*) parent;
   object_list_insert(command->list, command->position, command->obj);
   object_list_set_changed(command->list, command->changed);
}
