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

static CmdExecuteValue_t copy_command_execute(Command_t *parent);
static void copy_command_undo(Command_t *parent);

static CommandClass_t copy_command_class = {
   NULL,                        /* copy_command_destruct */
   copy_command_execute,
   copy_command_undo,
   NULL                         /* copy_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
   ObjectList_t *paste_buffer;
} CopyCommand_t;

Command_t*
copy_command_new(ObjectList_t *list)
{
   CopyCommand_t *command = g_new(CopyCommand_t, 1);
   command->list = list;
   command->paste_buffer = NULL;
   return command_init(&command->parent, _("Copy"), &copy_command_class);
}

static CmdExecuteValue_t
copy_command_execute(Command_t *parent)
{
   CopyCommand_t *command = (CopyCommand_t*) parent;
   command->paste_buffer = object_list_copy(command->paste_buffer,
                                            get_paste_buffer());
   object_list_copy_to_paste_buffer(command->list);
   return CMD_APPEND;
}

static void
copy_command_undo(Command_t *parent)
{
   CopyCommand_t *command = (CopyCommand_t*) parent;
   object_list_copy(get_paste_buffer(), command->paste_buffer);
}
