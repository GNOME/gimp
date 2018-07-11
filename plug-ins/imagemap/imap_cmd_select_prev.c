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

static CmdExecuteValue_t select_prev_command_execute(Command_t *parent);

static CommandClass_t select_prev_command_class = {
   NULL,                        /* select_prev_command_destruct */
   select_prev_command_execute,
   NULL,                        /* select_prev_command_undo */
   NULL                         /* select_prev_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
} SelectPrevCommand_t;

Command_t*
select_prev_command_new(ObjectList_t *list)
{
   SelectPrevCommand_t *command = g_new(SelectPrevCommand_t, 1);
   command->list = list;
   return command_init(&command->parent, _("Select Previous"),
                       &select_prev_command_class);
}

static void
select_one_object(Object_t *obj, gpointer data)
{
   SelectPrevCommand_t *command = (SelectPrevCommand_t*) data;
   Command_t *sub_command;

   sub_command = (obj->selected)
      ? select_command_new(obj) : unselect_command_new(obj);
   command_add_subcommand(&command->parent, sub_command);
}

static CmdExecuteValue_t
select_prev_command_execute(Command_t *parent)
{
   SelectPrevCommand_t *command = (SelectPrevCommand_t*) parent;
   ObjectList_t *list = command->list;
   gpointer id;

   id = object_list_add_select_cb(list, select_one_object, command);
   object_list_select_prev(list);
   object_list_remove_select_cb(list, id);
   return CMD_APPEND;
}
