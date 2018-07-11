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

static CmdExecuteValue_t move_down_command_execute(Command_t *parent);

static CommandClass_t move_down_command_class = {
   NULL,                        /* move_down_command_destruct */
   move_down_command_execute,
   NULL,                        /* move_down_command_undo */
   NULL                         /* move_down_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
   gboolean add;
} MoveDownCommand_t;

Command_t*
move_down_command_new(ObjectList_t *list)
{
   MoveDownCommand_t *command = g_new(MoveDownCommand_t, 1);
   command->list = list;
   command->add = FALSE;
   return command_init(&command->parent, _("Move Down"),
                       &move_down_command_class);
}

static void
move_down_one_object(Object_t *obj, gpointer data)
{
   MoveDownCommand_t *command = (MoveDownCommand_t*) data;

   if (command->add) {
      command_add_subcommand(&command->parent,
                             object_down_command_new(command->list, obj));
      command->add = FALSE;
   }
   else {
      command->add = TRUE;
   }
}

static CmdExecuteValue_t
move_down_command_execute(Command_t *parent)
{
   MoveDownCommand_t *command = (MoveDownCommand_t*) parent;
   gpointer id;

   id = object_list_add_move_cb(command->list, move_down_one_object, command);
   object_list_move_selected_down(command->list);
   object_list_remove_move_cb(command->list, id);

   return CMD_APPEND;
}
