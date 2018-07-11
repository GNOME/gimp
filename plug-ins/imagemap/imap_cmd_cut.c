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

static void cut_command_destruct(Command_t *parent);
static CmdExecuteValue_t cut_command_execute(Command_t *parent);
static void cut_command_undo(Command_t *parent);

static CommandClass_t cut_command_class = {
   cut_command_destruct,
   cut_command_execute,
   cut_command_undo,
   NULL                         /* cut_command_redo */
};

typedef struct {
   Command_t     parent;
   ObjectList_t *list;
   ObjectList_t *paste_buffer;
} CutCommand_t;

Command_t*
cut_command_new(ObjectList_t *list)
{
   CutCommand_t *command = g_new(CutCommand_t, 1);
   command->list = list;
   command->paste_buffer = NULL;
   return command_init(&command->parent, _("Cut"), &cut_command_class);
}

static void
cut_command_destruct(Command_t *parent)
{
   CutCommand_t *command = (CutCommand_t*) parent;
   object_list_destruct(command->paste_buffer);
}

static void
remove_one_object(Object_t *obj, gpointer data)
{
   CutCommand_t *command = (CutCommand_t*) data;
   command_add_subcommand(&command->parent,
                          delete_command_new(command->list, obj));
}

static CmdExecuteValue_t
cut_command_execute(Command_t *parent)
{
   CutCommand_t *command = (CutCommand_t*) parent;
   gpointer id;

   command->paste_buffer = object_list_copy(command->paste_buffer,
                                            get_paste_buffer());
   id = object_list_add_remove_cb(command->list, remove_one_object, command);
   object_list_cut(command->list);
   object_list_remove_remove_cb(command->list, id);

   return CMD_APPEND;
}

static void
cut_command_undo(Command_t *parent)
{
   CutCommand_t *command = (CutCommand_t*) parent;
   object_list_copy(get_paste_buffer(), command->paste_buffer);
}
