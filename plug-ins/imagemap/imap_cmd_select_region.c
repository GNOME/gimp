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
#include "imap_rectangle.h"
#include "imap_main.h"

#include "libgimp/stdplugins-intl.h"

static CmdExecuteValue_t select_region_command_execute(Command_t *parent);

static CommandClass_t select_region_command_class = {
   NULL,                        /* select_region_command_destruct, */
   select_region_command_execute,
   NULL,                        /* select_region_command_undo */
   NULL                         /* select_region_command_redo */
};

typedef struct {
   Command_t     parent;
   GtkWidget    *widget;
   ObjectList_t *list;
   gint          x;
   gint          y;
   Object_t     *obj;
   Command_t    *unselect_command;
} SelectRegionCommand_t;

Command_t*
select_region_command_new(GtkWidget *widget, ObjectList_t *list, gint x,
                          gint y)
{
   SelectRegionCommand_t *command = g_new(SelectRegionCommand_t, 1);
   Command_t *sub_command;

   command->widget = widget;
   command->list = list;
   command->x = x;
   command->y = y;
   (void) command_init(&command->parent, _("Select Region"),
                       &select_region_command_class);

   sub_command = unselect_all_command_new(list, NULL);
   command_add_subcommand(&command->parent, sub_command);
   command->unselect_command = sub_command;

   return &command->parent;
}

static void
select_one_object(Object_t *obj, gpointer data)
{
   SelectRegionCommand_t *command = (SelectRegionCommand_t*) data;
   command_add_subcommand(&command->parent, select_command_new(obj));
}

static void
select_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
   SelectRegionCommand_t *command = (SelectRegionCommand_t*) data;
   Object_t *obj = command->obj;
   Rectangle_t *rectangle = ObjectToRectangle(obj);

   gint x = get_real_coord((gint) event->x);
   gint y = get_real_coord((gint) event->y);

   rectangle->width = x - rectangle->x;
   rectangle->height = y - rectangle->y;

   preview_redraw ();
}

static void
select_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   SelectRegionCommand_t *command = (SelectRegionCommand_t*) data;
   Object_t *obj = command->obj;
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   gpointer id;
   gint count;

   g_signal_handlers_disconnect_by_func(widget,
                                        select_motion, data);
   g_signal_handlers_disconnect_by_func(widget,
                                        select_release, data);

   object_normalize(obj);

   id = object_list_add_select_cb(command->list, select_one_object, command);
   count = object_list_select_region(command->list, rectangle->x, rectangle->y,
                                     rectangle->width, rectangle->height);
   object_list_remove_select_cb(command->list, id);

   if (count) {
      command_list_add(&command->parent);
   } else {                     /* Nothing selected */
      if (command->unselect_command->sub_commands)
         command_list_add(&command->parent);
   }
   preview_unset_tmp_obj (command->obj);

   preview_redraw ();
}

static CmdExecuteValue_t
select_region_command_execute(Command_t *parent)
{
   SelectRegionCommand_t *command = (SelectRegionCommand_t*) parent;

   command->obj = create_rectangle(command->x, command->y, 0, 0);
   preview_set_tmp_obj (command->obj);

   g_signal_connect(command->widget, "button-release-event",
                    G_CALLBACK (select_release), command);
   g_signal_connect(command->widget, "motion-notify-event",
                    G_CALLBACK (select_motion), command);

   return CMD_IGNORE;
}
