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

#include <stdio.h>

#include "config.h"
#include "imap_cmd_edit_object.h"
#include "imap_cmd_select.h"
#include "imap_cmd_unselect.h"
#include "imap_cmd_unselect_all.h"
#include "imap_edit_area_info.h"
#include "libgimp/stdplugins-intl.h"
#include "imap_main.h"
#include "imap_misc.h"
#include "imap_selection.h"

#include "arrow_up.xpm"
#include "arrow_down.xpm"
#include "delete.xpm"
#include "edit.xpm"

static void
set_buttons(Selection_t *data)
{
   if (data->selected_child) {
      gtk_widget_set_sensitive(data->arrow_up, 
			       (data->selected_row) ? TRUE : FALSE);
      if (data->selected_row < GTK_CLIST(data->list)->rows - 1) 
	 gtk_widget_set_sensitive(data->arrow_down, TRUE);
      else
	 gtk_widget_set_sensitive(data->arrow_down, FALSE);
      gtk_widget_set_sensitive(data->remove, TRUE);
      gtk_widget_set_sensitive(data->edit, TRUE);
   } else {
      gtk_widget_set_sensitive(data->arrow_up, FALSE);
      gtk_widget_set_sensitive(data->arrow_down, FALSE);
      gtk_widget_set_sensitive(data->remove, FALSE);
      gtk_widget_set_sensitive(data->edit, FALSE);
   }
}

static void
select_row_cb(GtkWidget *widget, gint row, gint column, GdkEventButton *event,
	      Selection_t *data)
{
   data->selected_child = widget;
   data->selected_row = row;

   set_buttons(data);
   if (data->select_lock) {
      data->select_lock = FALSE;
   } else {
      Object_t *obj = gtk_clist_get_row_data(GTK_CLIST(data->list), row);
      Command_t *command;
      /* Note: event can be NULL if select_row_cb is called as a result of
	 a key press! */
      if (event && event->state & GDK_SHIFT_MASK) {
	 command = select_command_new(obj);
      } else {
	 Command_t *sub_command;
	 
	 command = subcommand_start(NULL);
	 sub_command = unselect_all_command_new(data->object_list, NULL);
	 command_add_subcommand(command, sub_command);
	 sub_command = select_command_new(obj);
	 command_add_subcommand(command, sub_command);
	 command_set_name(command, sub_command->name);
	 subcommand_end();
      }
      command_execute(command);
      redraw_preview();		/* Fix me! */
   }
}

static void
unselect_row_cb(GtkWidget *widget, gint row, gint column, 
		GdkEventButton *event, Selection_t *data)
{
   if (data->unselect_lock) {
      data->unselect_lock = FALSE;
   } else {
      Object_t *obj = gtk_clist_get_row_data(GTK_CLIST(data->list), row);
      Command_t *command;
      command = unselect_command_new(obj);
      command_execute(command);
      redraw_preview();		/* Fix me! */
   }
   data->selected_child = NULL;
   set_buttons(data);
}

static void
row_move_cb(GtkWidget *widget, gint src, gint des)
{
   printf("move: %d %d\n", src, des);
}

static gboolean doubleclick;

static void
button_press_cb(GtkWidget *widget, GdkEventButton *event, Selection_t *data)
{
   if (event->button == 1) {
      if (doubleclick) {
	 gint row, column;
	 doubleclick = FALSE;
	 if (gtk_clist_get_selection_info(GTK_CLIST(widget), (gint) event->x, 
					  (gint) event->y, &row, &column)) {
	    Object_t *obj = gtk_clist_get_row_data(GTK_CLIST(data->list), row);
	    object_edit(obj, TRUE);
	 }
      } else {
	 doubleclick = TRUE;
      }
   }
}

static void
button_release_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   if (event->button == 1)
      doubleclick = FALSE;
}

static void
selection_command(GtkWidget *widget, gpointer data)
{
   CommandFactory_t *factory = (CommandFactory_t*) data;
   Command_t *command = (*factory)();
   command_execute(command);
}

static GtkWidget*
make_selection_toolbar(Selection_t *data, GtkWidget *window)
{
   GtkWidget *toolbar;

   toolbar = gtk_toolbar_new(GTK_ORIENTATION_VERTICAL, GTK_TOOLBAR_ICONS);
   gtk_container_set_border_width(GTK_CONTAINER(toolbar), 5);
   gtk_toolbar_set_space_size(GTK_TOOLBAR(toolbar), 5);

   data->arrow_up = make_toolbar_icon(toolbar, window, arrow_up_xpm, "MoveUp",
				      _("Move Up"), selection_command, 
				      &data->cmd_move_up);
   data->arrow_down = make_toolbar_icon(toolbar, window, arrow_down_xpm, 
					"MoveDown", _("Move Down"), 
					selection_command, 
					&data->cmd_move_down);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   data->edit = make_toolbar_icon(toolbar, window, edit_xpm, "Edit",
				  _("Edit"), selection_command, 
				  &data->cmd_edit);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   data->remove = make_toolbar_icon(toolbar, window, delete_xpm, "Delete",
				    _("Delete"), selection_command, 
				    &data->cmd_delete);

   gtk_widget_show(toolbar);

   return toolbar;
}

static void
selection_update(Selection_t *selection, gint row, Object_t *obj)
{
   GdkBitmap *mask;
   GdkPixmap *icon = object_get_icon(obj, selection->list, &mask);

   gtk_clist_set_pixtext(GTK_CLIST(selection->list), row, 1, obj->url, 8,
			 icon, mask);
   gtk_clist_set_text(GTK_CLIST(selection->list), row, 2, obj->target);
   gtk_clist_set_text(GTK_CLIST(selection->list), row, 3, obj->comment);
}

static void
selection_renumber(Selection_t *selection, gint row)
{
   for (; row < selection->nr_rows; row++) {
      char scratch[16];
      sprintf(scratch, "%d", row + 1);
      gtk_clist_set_text(GTK_CLIST(selection->list), row, 0, scratch);
   }
}

static void
selection_set_selected(Selection_t *selection, gint row)
{
   Object_t *obj = gtk_clist_get_row_data(GTK_CLIST(selection->list), row);

   if (obj->selected) {
      selection->select_lock = TRUE;
      gtk_clist_select_row(GTK_CLIST(selection->list), row, -1);
   } else {
      selection->unselect_lock = TRUE;
      gtk_clist_unselect_row(GTK_CLIST(selection->list), row, -1);
   }
}

static void
object_added_cb(Object_t *obj, gpointer data)
{
   Selection_t *selection = (Selection_t*) data;
   gint row = object_get_position_in_list(obj);
   char scratch[16];
   gchar *text[4];

   selection->nr_rows++;
   
   text[0] = scratch;
   text[1] = obj->url;
   text[2] = obj->target;
   text[3] = obj->comment;
   
   if (row < selection->nr_rows - 1) {
      sprintf(scratch, "%d", row + 1);
      gtk_clist_insert(GTK_CLIST(selection->list), row, text);
      selection_renumber(selection, row);
   } else {
      sprintf(scratch, "%d", selection->nr_rows);
      gtk_clist_append(GTK_CLIST(selection->list), text);
   }
   selection_update(selection, row, obj);
   gtk_clist_set_row_data(GTK_CLIST(selection->list), row, (gpointer) obj);
   selection_set_selected(selection, row);
}

static gint
selection_find_object(Selection_t *selection, Object_t *obj)
{
   return gtk_clist_find_row_from_data(GTK_CLIST(selection->list),
				       (gpointer) obj);
}

static void
object_updated_cb(Object_t *obj, gpointer data)
{
   Selection_t *selection = (Selection_t*) data;
   gint row = selection_find_object(selection, obj);
   selection_update(selection, row, obj);
}

static void
object_removed_cb(Object_t *obj, gpointer data)
{
   Selection_t *selection = (Selection_t*) data;
   gint row = selection_find_object(selection, obj);

   selection->unselect_lock = TRUE;
   gtk_clist_unselect_row(GTK_CLIST(selection->list), row, -1);
   gtk_clist_remove(GTK_CLIST(selection->list), row);
   selection_renumber(selection, row);
   selection->selected_child = NULL;
   selection->nr_rows--;
   set_buttons(selection);
}

static void
object_selected_cb(Object_t *obj, gpointer data)
{
   Selection_t *selection = (Selection_t*) data;
   gint row = selection_find_object(selection, obj);

   selection_set_selected(selection, row);
}

static void
object_moved_cb(Object_t *obj, gpointer data)
{
   Selection_t *selection = (Selection_t*) data;
   gint row = object_get_position_in_list(obj);
   selection->select_lock = TRUE;
   gtk_clist_set_row_data(GTK_CLIST(selection->list), row, (gpointer) obj);
   selection_set_selected(selection, row);
   selection_update(selection, row, obj);
}

static void
toggle_order(GtkWidget *widget, gint column, gpointer data)
{
   /* Fix me! */
}

static GtkTargetEntry target_table[] = {
   {"STRING", 0, 1 },
   {"text/plain", 0, 2 }
};

static void 
handle_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, 
	    GtkSelectionData *data, guint info, guint time)
{
   gboolean success = FALSE;
   if (data->length >= 0 && data->format == 8) {
      gint row, column;
      if (gtk_clist_get_selection_info(GTK_CLIST(widget), x, y, &row, 
				       &column)) {
	 Object_t *obj = gtk_clist_get_row_data(GTK_CLIST(widget), row + 1);
	 if (!obj->locked) {
	    command_list_add(edit_object_command_new(obj));
	    object_set_url(obj, data->data);
	    object_emit_update_signal(obj);
	    success = TRUE;
	 }
      }
   }
   gtk_drag_finish(context, success, FALSE, time);
}

Selection_t*
make_selection(GtkWidget *window, ObjectList_t *object_list)
{
   Selection_t *data = g_new(Selection_t, 1);
   GtkWidget *swin, *frame, *hbox;
   GtkWidget *toolbar;
   GtkWidget *list;
   gchar     *titles[] = {"#", N_("URL"), N_("Target"), N_("Comment")};
   gint      i;

   data->object_list = object_list;
   data->selected_child = NULL;
   data->is_visible = TRUE;
   data->nr_rows = 0;
   data->select_lock = FALSE;
   data->unselect_lock = FALSE;

   data->container = frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
   gtk_widget_show(frame);

   hbox = gtk_hbox_new(FALSE, 1);
   gtk_container_add(GTK_CONTAINER(frame), hbox); 
   gtk_widget_show(hbox);

   toolbar = make_selection_toolbar(data, window);
   gtk_container_add(GTK_CONTAINER(hbox), toolbar);

   /* Create selection */
   frame = gtk_frame_new(_("Selection"));
   gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
   gtk_container_add(GTK_CONTAINER(hbox), frame);
   gtk_widget_show(frame);

   for (i = 0; i < 4; i++)
     titles[i] = gettext(titles[i]);
   data->list = list = gtk_clist_new_with_titles(4, titles);
   GTK_WIDGET_UNSET_FLAGS(data->list, GTK_CAN_FOCUS);
   gtk_clist_column_titles_passive(GTK_CLIST(list));
   gtk_clist_column_title_active(GTK_CLIST(list), 0);
   gtk_clist_set_column_width(GTK_CLIST(list), 0, 16);
   gtk_clist_set_column_width(GTK_CLIST(list), 1, 80);
   gtk_clist_set_column_width(GTK_CLIST(list), 2, 64);
   gtk_clist_set_column_width(GTK_CLIST(list), 3, 64);
/*   gtk_clist_set_reorderable(GTK_CLIST(list), TRUE); */

   /* Create scrollable window */
   swin = gtk_scrolled_window_new(NULL, NULL);
   gtk_widget_set_usize(swin, 16 + 80 + 2 * 64 + 16, -1);
   gtk_container_add(GTK_CONTAINER(frame), swin);
   gtk_widget_show(swin);

   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), list);
   gtk_widget_show(list);

   /* Drop support */
   gtk_drag_dest_set(list, GTK_DEST_DEFAULT_ALL, target_table,
		     2, GDK_ACTION_COPY);
   gtk_signal_connect(GTK_OBJECT(list), "drag_data_received",
		      GTK_SIGNAL_FUNC(handle_drop), NULL);

   /* Callbacks we are interested in */
   gtk_signal_connect(GTK_OBJECT(list), "click_column",
		      GTK_SIGNAL_FUNC(toggle_order), data);
   gtk_signal_connect(GTK_OBJECT(list), "select_row",
		      GTK_SIGNAL_FUNC(select_row_cb), data);
   gtk_signal_connect(GTK_OBJECT(list), "unselect_row",
		      GTK_SIGNAL_FUNC(unselect_row_cb), data);
   gtk_signal_connect(GTK_OBJECT(list), "row_move",
		      GTK_SIGNAL_FUNC(row_move_cb), data);

   /* For handling doubleclick */
   gtk_signal_connect(GTK_OBJECT(list), "button_press_event",
		      GTK_SIGNAL_FUNC(button_press_cb), data);
   gtk_signal_connect(GTK_OBJECT(list), "button_release_event",
		      GTK_SIGNAL_FUNC(button_release_cb), data);

   gtk_clist_set_selection_mode(GTK_CLIST(list), GTK_SELECTION_MULTIPLE);

   set_buttons(data);

   /* Set object list callbacks we're interested in */
   object_list_add_add_cb(object_list, object_added_cb, data);
   object_list_add_update_cb(object_list, object_updated_cb, data);
   object_list_add_remove_cb(object_list, object_removed_cb, data);
   object_list_add_select_cb(object_list, object_selected_cb, data);
   object_list_add_move_cb(object_list, object_moved_cb, data);

   return data;
}

void
selection_toggle_visibility(Selection_t *selection)
{
   if (selection->is_visible) {
      gtk_widget_hide(selection->container);
      selection->is_visible = FALSE;
   } else {
      gtk_widget_show(selection->container);
      selection->is_visible = TRUE;
   }
}
