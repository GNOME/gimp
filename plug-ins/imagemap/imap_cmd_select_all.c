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

static CmdExecuteValue_t select_all_command_execute(Command_t *parent);

static CommandClass_t select_all_command_class = {
   NULL,                        /* select_all_command_destruct, */
   select_all_command_execute,
   NULL,                        /* select_all_command_undo */
   NULL                         /* select_all_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
} SelectAllCommand_t;

Command_t*
select_all_command_new(ObjectList_t *list)
{
   SelectAllCommand_t *command = g_new(SelectAllCommand_t, 1);
   command->list = list;
   return command_init(&command->parent, _("Select All"),
                       &select_all_command_class);
}

static void
select_one_object(Object_t *obj, gpointer data)
{
   SelectAllCommand_t *command = (SelectAllCommand_t*) data;
   command_add_subcommand(&command->parent, select_command_new(obj));
}

static CmdExecuteValue_t
select_all_command_execute(Command_t *parent)
{
   SelectAllCommand_t *command = (SelectAllCommand_t*) parent;
   gpointer id;
   CmdExecuteValue_t rvalue;

   id = object_list_add_select_cb(command->list, select_one_object, command);
   rvalue = (object_list_select_all(command->list))
     ? CMD_APPEND : CMD_DESTRUCT;
   object_list_remove_select_cb(command->list, id);

   return rvalue;
}
