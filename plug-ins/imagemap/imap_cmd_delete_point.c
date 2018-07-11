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
#include "imap_polygon.h"

#include "libgimp/stdplugins-intl.h"

static CmdExecuteValue_t delete_point_command_execute(Command_t *parent);
static void delete_point_command_undo(Command_t *parent);

static CommandClass_t delete_point_command_class = {
   NULL,                        /* delete_point_command_destruct */
   delete_point_command_execute,
   delete_point_command_undo,
   NULL                         /* delete_point_command_redo */
};

typedef struct {
   Command_t    parent;
   Polygon_t   *polygon;
   GdkPoint    *point;
   GdkPoint     copy;
   gint         position;
} DeletePointCommand_t;

Command_t*
delete_point_command_new(Object_t *obj, GdkPoint *point)
{
   DeletePointCommand_t *command = g_new(DeletePointCommand_t, 1);

   command->polygon = ObjectToPolygon(obj);
   command->point = point;
   command->copy = *point;
   command->position = g_list_index(command->polygon->points,
                                    (gpointer) point);
   return command_init(&command->parent, _("Delete Point"),
                       &delete_point_command_class);
}

static CmdExecuteValue_t
delete_point_command_execute(Command_t *parent)
{
   DeletePointCommand_t *command = (DeletePointCommand_t*) parent;
   Polygon_t *polygon = command->polygon;
   GList *p = g_list_find(polygon->points, (gpointer) command->point);

   g_free(p->data);
   polygon->points = g_list_remove_link(polygon->points, p);
   return CMD_APPEND;
}

static void
delete_point_command_undo(Command_t *parent)
{
   DeletePointCommand_t *command = (DeletePointCommand_t*) parent;
   Polygon_t *polygon = command->polygon;
   GdkPoint *point = &command->copy;

   command->point = new_point(point->x, point->y);
   polygon->points = g_list_insert(polygon->points, (gpointer) command->point,
                                   command->position);
}
