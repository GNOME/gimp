/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "imap_cmd_copy.h"
#include "imap_main.h"

static void move_command_destruct(Command_t *parent);
static gboolean move_command_execute(Command_t *parent);
static void move_command_undo(Command_t *parent);

CommandClass_t move_command_class = {
   move_command_destruct,
   move_command_execute,
   move_command_undo,
   NULL				/* move_command_redo */
};

typedef struct {
   Command_t parent;
   Object_t *obj;
   gint dx;
   gint dy;
} MoveCommand_t;

Command_t* 
move_command_new(Object_t *obj, gint dx, gint dy)
{
   MoveCommand_t *command = g_new(MoveCommand_t, 1);
   command->obj = object_ref(obj);
   command->dx = dx;
   command->dy = dy;
   return command_init(&command->parent, "Move", &move_command_class);
}

static void
move_command_destruct(Command_t *parent)
{
   MoveCommand_t *command = (MoveCommand_t*) parent;
   object_unref(command->obj);
}

static gboolean
move_command_execute(Command_t *parent)
{
   MoveCommand_t *command = (MoveCommand_t*) parent;
   object_move(command->obj, command->dx, command->dy);
   redraw_preview();		/* fix me! */
   return TRUE;
}

static void
move_command_undo(Command_t *parent)
{
   MoveCommand_t *command = (MoveCommand_t*) parent;
   object_move(command->obj, -command->dx, -command->dy);
   redraw_preview();		/* fix me! */
}

