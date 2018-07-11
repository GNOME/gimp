/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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

static void edit_object_command_destruct(Command_t *parent);
static void edit_object_command_undo(Command_t *parent);

static CommandClass_t edit_object_command_class = {
   edit_object_command_destruct,
   NULL,                        /* edit_object_command_execute */
   edit_object_command_undo,
   edit_object_command_undo
};

typedef struct {
   Command_t parent;
   Object_t *obj;
   Object_t *copy;
} EditObjectCommand_t;

Command_t*
edit_object_command_new(Object_t *obj)
{
   EditObjectCommand_t *command = g_new(EditObjectCommand_t, 1);
   command->obj = object_ref(obj);
   command->copy = object_clone(obj);
   return command_init(&command->parent, _("Edit Object"),
                       &edit_object_command_class);
}

static void
edit_object_command_destruct(Command_t *parent)
{
   EditObjectCommand_t *command = (EditObjectCommand_t*) parent;
   object_unref(command->copy);
   object_unref(command->obj);
}

static void
edit_object_command_undo(Command_t *parent)
{
   EditObjectCommand_t *command = (EditObjectCommand_t*) parent;
   Object_t *copy = object_clone(command->obj);

   object_assign(command->copy, command->obj);
   object_assign(copy, command->copy);
}
