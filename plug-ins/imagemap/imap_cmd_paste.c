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

static CmdExecuteValue_t paste_command_execute(Command_t *parent);

static CommandClass_t paste_command_class = {
   NULL,                        /* paste_command_destruct, */
   paste_command_execute,
   NULL,                        /* paste_command_undo */
   NULL                         /* paste_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
} PasteCommand_t;

Command_t*
paste_command_new(ObjectList_t *list)
{
   PasteCommand_t *command = g_new(PasteCommand_t, 1);
   command->list = list;
   return command_init(&command->parent, _("Paste"), &paste_command_class);
}

static void
paste_one_object(Object_t *obj, gpointer data)
{
   PasteCommand_t *command = (PasteCommand_t*) data;
   command_add_subcommand(&command->parent,
                          create_command_new(command->list, obj));
}

static CmdExecuteValue_t
paste_command_execute(Command_t *parent)
{
   PasteCommand_t *command = (PasteCommand_t*) parent;
   gpointer id;

   id = object_list_add_add_cb(command->list, paste_one_object, command);
   object_list_paste(command->list);
   object_list_remove_add_cb(command->list, id);
   return CMD_APPEND;
}
