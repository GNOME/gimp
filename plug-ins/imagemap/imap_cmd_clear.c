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

static CmdExecuteValue_t clear_command_execute(Command_t *parent);

static CommandClass_t clear_command_class = {
   NULL,                        /* clear_command_destruct */
   clear_command_execute,
   NULL,                        /* clear_command_undo */
   NULL                         /* clear_command_redo */
};

typedef struct {
   Command_t     parent;
   ObjectList_t *list;
} ClearCommand_t;

Command_t*
clear_command_new(ObjectList_t *list)
{
   ClearCommand_t *command = g_new(ClearCommand_t, 1);
   command->list = list;
   return command_init(&command->parent, _("Clear"), &clear_command_class);
}

static void
remove_one_object(Object_t *obj, gpointer data)
{
   ClearCommand_t *command = (ClearCommand_t*) data;
   command_add_subcommand(&command->parent,
                          delete_command_new(command->list, obj));
}

static CmdExecuteValue_t
clear_command_execute(Command_t *parent)
{
   ClearCommand_t *command = (ClearCommand_t*) parent;
   gpointer id;

   id = object_list_add_remove_cb(command->list, remove_one_object, command);
   object_list_delete_selected(command->list);
   object_list_remove_remove_cb(command->list, id);

   return CMD_APPEND;
}
