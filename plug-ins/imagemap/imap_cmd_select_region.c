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

#include "imap_rectangle.h"
#include "imap_cmd_select.h"
#include "imap_cmd_select_region.h"
#include "imap_cmd_unselect_all.h"
#include "imap_main.h"

static gboolean select_region_command_execute(Command_t *parent);
static void select_region_command_undo(Command_t *parent);
static void select_region_command_redo(Command_t *parent);

static CommandClass_t select_region_command_class = {
   NULL,			/* select_region_command_destruct, */
   select_region_command_execute,
   select_region_command_undo,
   select_region_command_redo
};

typedef struct {
   Command_t	 parent;
   GtkWidget	*widget;
   ObjectList_t *list;
   gint		 x;
   gint		 y;
   Object_t	*obj;
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
   (void) command_init(&command->parent, "Select Region", 
		       &select_region_command_class);

   sub_command = unselect_all_command_new(list, NULL);
   command_add_subcommand(&command->parent, sub_command);

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

   object_draw(obj, widget->window);
   rectangle->width = x - rectangle->x;
   rectangle->height = y - rectangle->y;
   object_draw(obj, widget->window);
}

static void
select_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   SelectRegionCommand_t *command = (SelectRegionCommand_t*) data;
   Object_t *obj = command->obj;
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   gpointer id;

   gtk_signal_disconnect_by_func(GTK_OBJECT(widget), 
				 (GtkSignalFunc) select_motion, data);
   gtk_signal_disconnect_by_func(GTK_OBJECT(widget), 
				 (GtkSignalFunc) select_release, data);

   object_draw(obj, widget->window);
   object_normalize(obj);
   gdk_gc_set_function(get_preferences()->normal_gc, GDK_COPY);

   id = object_list_add_select_cb(command->list, select_one_object, command);
   if (object_list_select_region(command->list, rectangle->x, rectangle->y,
				 rectangle->width, rectangle->height))
      redraw_preview();
   object_list_remove_select_cb(command->list, id);

   object_unref(obj);
}

static gboolean
select_region_command_execute(Command_t *parent)
{
   SelectRegionCommand_t *command = (SelectRegionCommand_t*) parent;
/*   Command_t *sub_command; */

   command->obj = create_rectangle(command->x, command->y, 0, 0);
   gtk_signal_connect(GTK_OBJECT(command->widget), "button_release_event", 
		      (GtkSignalFunc) select_release, command);   
   gtk_signal_connect(GTK_OBJECT(command->widget), "motion_notify_event", 
		      (GtkSignalFunc) select_motion, command);   

#ifdef _OLD_
   sub_command = unselect_all_command_new(command->list, NULL);
   command_add_subcommand(parent, sub_command);
   command_execute(sub_command);
#endif
   gdk_gc_set_function(get_preferences()->normal_gc, GDK_EQUIV);

   return TRUE;
}

static void
select_region_command_undo(Command_t *command)
{
   redraw_preview();		/* Fix me! */
}

static void
select_region_command_redo(Command_t *command)
{
   redraw_preview();		/* Fix me! */
}


