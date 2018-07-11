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

static CmdExecuteValue_t move_selected_command_execute(Command_t *parent);
static void move_selected_command_undo(Command_t *parent);

static CommandClass_t move_selected_command_class = {
   NULL,                        /* move_selected_command_destruct */
   move_selected_command_execute,
   move_selected_command_undo,
   NULL                         /* move_selected_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
   gint dx;
   gint dy;
} MoveSelectedCommand_t;

Command_t*
move_selected_command_new(ObjectList_t *list, gint dx, gint dy)
{
   MoveSelectedCommand_t *command = g_new(MoveSelectedCommand_t, 1);
   command->list = list;
   command->dx = dx;
   command->dy = dy;
   return command_init(&command->parent, _("Move Selected Objects"),
                       &move_selected_command_class);
}

static CmdExecuteValue_t
move_selected_command_execute(Command_t *parent)
{
   MoveSelectedCommand_t *command = (MoveSelectedCommand_t*) parent;
   object_list_move_selected(command->list, command->dx, command->dy);
   return CMD_APPEND;
}

static void
move_selected_command_undo(Command_t *parent)
{
   MoveSelectedCommand_t *command = (MoveSelectedCommand_t*) parent;
   object_list_move_selected(command->list, -command->dx, -command->dy);
}
