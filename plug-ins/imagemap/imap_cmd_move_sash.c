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
#include "imap_cmd_edit_object.h"
#include "imap_main.h"

COMMAND_PROTO(move_sash_command);

CommandClass_t move_sash_command_class = {
   move_sash_command_destruct,
   move_sash_command_execute,
   move_sash_command_undo,
   move_sash_command_redo
};

typedef struct {
   Command_t 	parent;
   GtkWidget   *widget;
   Object_t    *obj;
   gint 	x;
   gint 	y;
   gint		image_width;
   gint		image_height;
   MoveSashFunc_t sash_func;
} MoveSashCommand_t;

Command_t* 
move_sash_command_new(GtkWidget *widget, Object_t *obj, 
		      gint x, gint y, MoveSashFunc_t sash_func)
{
   MoveSashCommand_t *command = g_new(MoveSashCommand_t, 1);
   Command_t *parent;

   command->widget = widget;
   command->obj = obj;
   command->x = x;
   command->y = y;
   command->image_width = get_image_width();
   command->image_height = get_image_height();
   command->sash_func = sash_func;

   parent = command_init(&command->parent, "Move Sash",
			 &move_sash_command_class);
   command_add_subcommand(parent, edit_object_command_new(obj));

   return parent;
}

static void
move_sash_command_destruct(Command_t *parent)
{
   MoveSashCommand_t *command = (MoveSashCommand_t*) parent;
   object_unref(command->obj);
}

static void
sash_move(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
   MoveSashCommand_t *command = (MoveSashCommand_t*) data;
   Object_t *obj = command->obj;
   gint x, y, dx, dy;

   x = (gint) event->x;
   y = (gint) event->y;

   if (x < 0)
      x = 0;
   if (x > command->image_width)
      x = command->image_width;

   if (y < 0)
      y = 0;
   if (y > command->image_height)
      y = command->image_height;

   x = get_real_coord(x);
   y = get_real_coord(y);

   dx = x - command->x;
   dy = y - command->y;

   command->x = x;
   command->y = y;

   object_draw(obj, widget->window);
   command->sash_func(obj, dx, dy);
   object_draw(obj, widget->window);
}

static void
sash_end(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   MoveSashCommand_t *command = (MoveSashCommand_t*) data;
   Object_t *obj = command->obj;

   gtk_signal_disconnect_by_func(GTK_OBJECT(widget), 
				 (GtkSignalFunc) sash_move, data);
   gtk_signal_disconnect_by_func(GTK_OBJECT(widget), 
				 (GtkSignalFunc) sash_end, data);
   if (obj->class->normalize)
      object_normalize(obj);
   gdk_gc_set_function(get_preferences()->selected_gc, GDK_COPY);
   redraw_preview();
   show_url();
}

static gboolean
move_sash_command_execute(Command_t *parent)
{
   MoveSashCommand_t *command = (MoveSashCommand_t*) parent;

   hide_url();
   gtk_signal_connect(GTK_OBJECT(command->widget), "button_release_event",
		      (GtkSignalFunc) sash_end, command);   
   gtk_signal_connect(GTK_OBJECT(command->widget), "motion_notify_event", 
		      (GtkSignalFunc) sash_move, command);   
   gdk_gc_set_function(get_preferences()->selected_gc, GDK_EQUIV);

   return TRUE;
}

static void
move_sash_command_undo(Command_t *parent)
{
}

static void
move_sash_command_redo(Command_t *parent)
{
}

