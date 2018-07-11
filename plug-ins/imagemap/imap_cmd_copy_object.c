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

static void copy_object_command_destruct(Command_t *parent);
static CmdExecuteValue_t copy_object_command_execute(Command_t *parent);
static void copy_object_command_undo(Command_t *parent);

static CommandClass_t copy_object_command_class = {
   copy_object_command_destruct,
   copy_object_command_execute,
   copy_object_command_undo,
   NULL                         /* copy_object_command_redo */
};

typedef struct {
   Command_t parent;
   Object_t     *obj;
   ObjectList_t *paste_buffer;
} CopyObjectCommand_t;

Command_t*
copy_object_command_new(Object_t *obj)
{
   CopyObjectCommand_t *command = g_new(CopyObjectCommand_t, 1);
   command->obj = object_ref(obj);
   command->paste_buffer = NULL;
   return command_init(&command->parent, _("Copy"),
                       &copy_object_command_class);
}

static void
copy_object_command_destruct(Command_t *parent)
{
   CopyObjectCommand_t *command = (CopyObjectCommand_t*) parent;
   object_unref(command->obj);
}

static CmdExecuteValue_t
copy_object_command_execute(Command_t *parent)
{
   CopyObjectCommand_t *command = (CopyObjectCommand_t*) parent;
   ObjectList_t *paste_buffer = get_paste_buffer();

   command->paste_buffer = object_list_copy(command->paste_buffer,
                                            paste_buffer);
   clear_paste_buffer();
   object_list_append(paste_buffer, object_clone(command->obj));

   return CMD_APPEND;
}

static void
copy_object_command_undo(Command_t *parent)
{
   CopyObjectCommand_t *command = (CopyObjectCommand_t*) parent;
   object_list_copy(get_paste_buffer(), command->paste_buffer);
}
