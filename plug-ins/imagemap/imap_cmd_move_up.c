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

static CmdExecuteValue_t move_up_command_execute(Command_t *parent);

static CommandClass_t move_up_command_class = {
   NULL,                        /* move_up_command_destruct */
   move_up_command_execute,
   NULL,                        /* move_up_command_undo */
   NULL                         /* move_up_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
   gboolean add;
} MoveUpCommand_t;

Command_t*
move_up_command_new(ObjectList_t *list)
{
   MoveUpCommand_t *command = g_new(MoveUpCommand_t, 1);
   command->list = list;
   command->add = FALSE;
   return command_init(&command->parent, _("Move Up"), &move_up_command_class);
}

static void
move_up_one_object(Object_t *obj, gpointer data)
{
   MoveUpCommand_t *command = (MoveUpCommand_t*) data;

   if (command->add) {
      command_add_subcommand(&command->parent,
                             object_up_command_new(command->list, obj));
      command->add = FALSE;
   }
   else {
      command->add = TRUE;
   }
}

static CmdExecuteValue_t
move_up_command_execute(Command_t *parent)
{
   MoveUpCommand_t *command = (MoveUpCommand_t*) parent;
   gpointer id;

   id = object_list_add_move_cb(command->list, move_up_one_object, command);
   object_list_move_selected_up(command->list);
   object_list_remove_move_cb(command->list, id);

   return CMD_APPEND;
}
