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

#include "imap_main.h"

#include "libgimp/stdplugins-intl.h"

static CmdExecuteValue_t create_command_execute(Command_t *parent);
static void create_command_destruct(Command_t *parent);
static void create_command_undo(Command_t *parent);

static CommandClass_t create_command_class = {
   create_command_destruct,
   create_command_execute,
   create_command_undo,
   NULL                         /* create_command_redo */
};

typedef struct {
   Command_t     parent;
   ObjectList_t *list;
   Object_t     *obj;
   gboolean      changed;
} CreateCommand_t;

Command_t*
create_command_new(ObjectList_t *list, Object_t *obj)
{
   CreateCommand_t *command = g_new(CreateCommand_t, 1);
   command->list = list;
   command->obj = object_ref(obj);
   return command_init(&command->parent, _("Create"), &create_command_class);
}

static void
create_command_destruct(Command_t *parent)
{
   CreateCommand_t *command = (CreateCommand_t*) parent;
   object_unref(command->obj);
}

static CmdExecuteValue_t
create_command_execute(Command_t *parent)
{
   CreateCommand_t *command = (CreateCommand_t*) parent;
   command->changed = object_list_get_changed(command->list);
   object_list_append(command->list, object_ref(command->obj));
   return CMD_APPEND;
}

static void
create_command_undo(Command_t *parent)
{
   CreateCommand_t *command = (CreateCommand_t*) parent;
   object_list_remove(command->list, command->obj);
   object_list_set_changed(command->list, command->changed);
}
