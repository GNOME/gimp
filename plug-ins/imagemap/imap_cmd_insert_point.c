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
#include "imap_polygon.h"

#include "libgimp/stdplugins-intl.h"

static CmdExecuteValue_t insert_point_command_execute(Command_t *parent);
static void insert_point_command_undo(Command_t *parent);

static CommandClass_t insert_point_command_class = {
   NULL,                        /* insert_point_command_destruct */
   insert_point_command_execute,
   insert_point_command_undo,
   NULL                         /* insert_point_command_redo */
};

typedef struct {
   Command_t    parent;
   Polygon_t   *polygon;
   gint         x;
   gint         y;
   gint         edge;
   gint         position;
} InsertPointCommand_t;

Command_t*
insert_point_command_new(Object_t *obj, gint x, gint y, gint edge)
{
   InsertPointCommand_t *command = g_new(InsertPointCommand_t, 1);

   command->polygon = ObjectToPolygon(obj);
   command->x = x;
   command->y = y;
   command->edge = edge;
   return command_init(&command->parent, _("Insert Point"),
                       &insert_point_command_class);
}

static CmdExecuteValue_t
insert_point_command_execute(Command_t *parent)
{
   InsertPointCommand_t *command = (InsertPointCommand_t*) parent;
   Polygon_t *polygon = command->polygon;

   GdkPoint *point = new_point(command->x, command->y);

   if (g_list_length(polygon->points) == command->edge - 1) {
      polygon->points = g_list_append(polygon->points, (gpointer) point);
      command->position = command->edge - 1;
   } else {
      polygon->points = g_list_insert(polygon->points, (gpointer) point,
                                      command->edge);
      command->position = command->edge;
   }
   preview_redraw();

   return CMD_APPEND;
}

static void
insert_point_command_undo(Command_t *parent)
{
   InsertPointCommand_t *command = (InsertPointCommand_t*) parent;
   Polygon_t *polygon = command->polygon;
   GList *p = g_list_nth(polygon->points, command->position);

   g_free(p->data);
   polygon->points = g_list_remove_link(polygon->points, p);
   preview_redraw();            /* Fix me! */
}
