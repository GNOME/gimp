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

#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "imap_cmd_delete_point.h"
#include "imap_cmd_insert_point.h"
#include "libgimp/stdplugins-intl.h"
#include "imap_main.h"
#include "imap_misc.h"
#include "imap_object_popup.h"
#include "imap_polygon.h"
#include "imap_table.h"

#include "polygon.xpm"

#define MAX_POLYGON_POINTS 99

static gboolean polygon_is_valid(Object_t *obj);
static void polygon_destruct(Object_t *obj);
static Object_t *polygon_clone(Object_t *obj);
static void polygon_assign(Object_t *obj, Object_t *des);
static void polygon_draw(Object_t* obj, GdkWindow *window, GdkGC* gc);
static void polygon_draw_sashes(Object_t* obj, GdkWindow *window, GdkGC* gc);
static MoveSashFunc_t polygon_near_sash(Object_t *obj, gint x, gint y);
static gboolean polygon_point_is_on(Object_t *obj, gint x, gint y);
static void polygon_get_dimensions(Object_t *obj, gint *x, gint *y,
				   gint *width, gint *height);
static void polygon_resize(Object_t *obj, gint percentage_x,
			   gint percentage_y);
static void polygon_move(Object_t *obj, gint dx, gint dy);
static gpointer polygon_create_info_widget(GtkWidget *frame);
static void polygon_update_info_widget(Object_t *obj, gpointer data);
static void polygon_fill_info_tab(Object_t *obj, gpointer data);
static void polygon_set_initial_focus(Object_t *obj, gpointer data);
static void polygon_update(Object_t* obj, gpointer data);
static void polygon_write_csim(Object_t* obj, gpointer param, 
			       OutputFunc_t output);
static void polygon_write_cern(Object_t* obj, gpointer param, 
			       OutputFunc_t output);
static void polygon_write_ncsa(Object_t* obj, gpointer param, 
			       OutputFunc_t output);
static void polygon_do_popup(Object_t *obj, GdkEventButton *event);
static char** polygon_get_icon_data(void);

static ObjectClass_t polygon_class = {
   N_("Polygon"),
   NULL,			/* info_dialog */
   NULL,			/* icon */
   NULL,			/* mask */

   polygon_is_valid,
   polygon_destruct,
   polygon_clone,
   polygon_assign,
   NULL,			/* polygon_normalize */
   polygon_draw,
   polygon_draw_sashes,
   polygon_near_sash,
   polygon_point_is_on,
   polygon_get_dimensions,
   polygon_resize,
   polygon_move,
   polygon_create_info_widget,
   polygon_update_info_widget,
   polygon_fill_info_tab,
   polygon_set_initial_focus,
   polygon_update,
   polygon_write_csim,
   polygon_write_cern,
   polygon_write_ncsa,
   polygon_do_popup,
   polygon_get_icon_data
};

Object_t*
create_polygon(GList *points)
{
   Polygon_t *polygon = g_new(Polygon_t, 1);
   polygon->points = points;
   return object_init(&polygon->obj, &polygon_class);
}

static void
polygon_free_list(Polygon_t *polygon)
{
   g_list_foreach(polygon->points, (GFunc) g_free, NULL);
   g_list_free(polygon->points);
   polygon->points = NULL;
}

static void
polygon_destruct(Object_t *obj)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   polygon_free_list(polygon);
}

static gboolean 
polygon_is_valid(Object_t *obj)
{
   return g_list_length(ObjectToPolygon(obj)->points) > 2;
}

static Object_t*
polygon_clone(Object_t *obj)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   Polygon_t *clone = g_new(Polygon_t, 1);
   GList     *p;
   
   clone->points = NULL;
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      clone->points = g_list_append(clone->points, 
				    new_point(point->x, point->y));
   }
   return &clone->obj;
}

static void
polygon_assign(Object_t *obj, Object_t *des)
{
   Polygon_t *src_polygon = ObjectToPolygon(obj);
   Polygon_t *des_polygon = ObjectToPolygon(des);
   GList     *p;

   polygon_free_list(des_polygon);
   for (p = src_polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      des_polygon->points = g_list_append(des_polygon->points, 
					  new_point(point->x, point->y));
   }
}

static void
polygon_draw(Object_t *obj, GdkWindow *window, GdkGC *gc)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   draw_polygon(window, gc, polygon->points);
}

static void
polygon_draw_sashes(Object_t *obj, GdkWindow *window, GdkGC *gc)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      draw_sash(window, gc, point->x, point->y);
   }
}

static GdkPoint *_sash_point;
static gint _sash_index;

static void
move_sash(Object_t *obj, gint dx, gint dy)
{
   _sash_point->x += dx;
   _sash_point->y += dy;
}

static MoveSashFunc_t
polygon_near_sash(Object_t *obj, gint x, gint y)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;

   _sash_index = 0;
   for (p = polygon->points; p; p = p->next, _sash_index++) {
      GdkPoint *point = (GdkPoint*) p->data;
      if (near_sash(point->x, point->y, x, y)) {
	 _sash_point = point;
	 return move_sash;
      }
   }
   return NULL;
}

static gboolean
right_intersect(GdkPoint *p1, GdkPoint *p2, gint x, gint y)
{
   gint dx = p2->x - p1->x;
   gint dy = p2->y - p1->y;

   if ((dy > 0 && y > p1->y && y < p2->y) ||
       (dy < y && y > p2->y && y < p1->y)) {
      gint sx = p1->x + (y - p1->y) * dx / dy;
      return sx > x;
   }
   return FALSE;
}

static gboolean
polygon_point_is_on(Object_t *obj, gint x, gint y)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;
   int        count = 0;
   GdkPoint  *first, *prev;

   p = polygon->points;
   first = prev = (GdkPoint*) p->data;
   p = p->next;

   for (; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      if (right_intersect(prev, point, x, y))
	 count++;
      prev = point;
   }
   if (right_intersect(prev, first, x, y))
       count++;

   return count % 2;
}

static void 
polygon_get_dimensions(Object_t *obj, gint *x, gint *y,
		       gint *width, gint *height)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   gint min_x = G_MAXINT, min_y = G_MAXINT;
   gint max_x = G_MININT, max_y = G_MININT;
   GList     *p;

   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      if (point->x < min_x)
	 min_x = point->x;
      if (point->x > max_x)
	 max_x = point->x;
      if (point->y < min_y)
	 min_y = point->y;
      if (point->y > max_y)
	 max_y = point->y;
   }
   *x = min_x;
   *y = min_y;
   *width = max_x - min_x;
   *height = max_y - min_y;
}

static void 
polygon_resize(Object_t *obj, gint percentage_x, gint percentage_y)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      point->x = point->x * percentage_x / 100;
      point->y = point->y * percentage_y / 100;
   }
}

static void
polygon_move(Object_t *obj, gint dx, gint dy)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      point->x += dx;
      point->y += dy;
   }
}

typedef struct {
   Object_t  *obj;
   GtkWidget *list;
   GtkWidget *x;
   GtkWidget *y;
   GtkWidget *update;
   GtkWidget *insert;
   GtkWidget *append;
   GtkWidget *remove;
   gint	      selected_row;
   gint	      timeout;
} PolygonProperties_t;

static void
select_row_cb(GtkWidget *widget, gint row, gint column, GdkEventButton *event,
	      PolygonProperties_t *data)
{
   gchar *text;
   data->selected_row = row;

   _sash_point = g_list_nth(ObjectToPolygon(data->obj)->points, row)->data;

   gtk_clist_get_text(GTK_CLIST(data->list), row, 0, &text);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->x), atoi(text));

   gtk_clist_get_text(GTK_CLIST(data->list), row, 1, &text);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->y), atoi(text));
}

static void
update_button_clicked(GtkWidget *widget, PolygonProperties_t *data)
{
   gtk_clist_set_text(GTK_CLIST(data->list), data->selected_row, 0,
		      gtk_entry_get_text(GTK_ENTRY(data->x)));
   gtk_clist_set_text(GTK_CLIST(data->list), data->selected_row, 1,
		      gtk_entry_get_text(GTK_ENTRY(data->y)));
}

static void
set_buttons_sensitivity(PolygonProperties_t *data)
{
   gint rows = GTK_CLIST(data->list)->rows;
   gint sensitive = (rows != MAX_POLYGON_POINTS) && (rows > 2);
   gtk_widget_set_sensitive(data->insert, sensitive);
   gtk_widget_set_sensitive(data->append, sensitive);
}

static void
insert_button_clicked(GtkWidget *widget, PolygonProperties_t *data)
{
   gchar *text[2];
   text[0] = gtk_entry_get_text(GTK_ENTRY(data->x));
   text[1] = gtk_entry_get_text(GTK_ENTRY(data->y));
   gtk_clist_insert(GTK_CLIST(data->list), data->selected_row, text);
   set_buttons_sensitivity(data);
}

static void
append_button_clicked(GtkWidget *widget, PolygonProperties_t *data)
{
   gchar *text[2];
   text[0] = gtk_entry_get_text(GTK_ENTRY(data->x));
   text[1] = gtk_entry_get_text(GTK_ENTRY(data->y));
   gtk_clist_append(GTK_CLIST(data->list), text);
   set_buttons_sensitivity(data);
}

static void
remove_button_clicked(GtkWidget *widget, PolygonProperties_t *data)
{
   gtk_clist_remove(GTK_CLIST(data->list), data->selected_row);
   set_buttons_sensitivity(data);
}

static void
x_changed_cb(GtkWidget *widget, gpointer data)
{
   Object_t *obj = ((PolygonProperties_t*) data)->obj;
   gint x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _sash_point->x = x;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static void
y_changed_cb(GtkWidget *widget, gpointer data)
{
   Object_t *obj = ((PolygonProperties_t*) data)->obj;
   gint y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _sash_point->y = y;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static gpointer
polygon_create_info_widget(GtkWidget *frame)
{
   PolygonProperties_t *props = g_new(PolygonProperties_t, 1);
   GtkWidget *hbox, *swin, *table;
   GtkWidget *list;
   gint max_width = get_image_width();
   gint max_height = get_image_height();
   gchar *titles[] = {N_("x (pixels)"), N_("y (pixels)")};
   gint i;

   hbox = gtk_hbox_new(FALSE, 1);
   gtk_container_add(GTK_CONTAINER(frame), hbox);
   gtk_widget_show(hbox);

   for (i = 0; i < 2; i++)
     titles[i] = gettext(titles[i]);
   props->list = list = gtk_clist_new_with_titles(2, titles);
   gtk_clist_column_titles_passive(GTK_CLIST(list));

   swin = gtk_scrolled_window_new(NULL, NULL);
   gtk_container_set_border_width(GTK_CONTAINER(swin), 10);

   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), 
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

   gtk_box_pack_start(GTK_BOX(hbox), swin, FALSE, FALSE, FALSE);
   gtk_widget_show(swin);

   gtk_clist_set_shadow_type(GTK_CLIST(list), GTK_SHADOW_ETCHED_IN);
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), list);

   gtk_clist_set_column_width(GTK_CLIST(list), 0, 48);
   gtk_clist_set_column_width(GTK_CLIST(list), 1, 48);
   gtk_clist_set_column_justification(GTK_CLIST(list), 0, GTK_JUSTIFY_RIGHT);
   gtk_clist_set_column_justification(GTK_CLIST(list), 1, GTK_JUSTIFY_RIGHT);

   gtk_signal_connect(GTK_OBJECT(list), "select_row",
		      GTK_SIGNAL_FUNC(select_row_cb), props);
   gtk_clist_set_selection_mode(GTK_CLIST(list), GTK_SELECTION_SINGLE);
   gtk_widget_show(list);

   table = gtk_table_new(6, 3, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_table_set_row_spacings(GTK_TABLE(table), 10);
   gtk_table_set_col_spacings(GTK_TABLE(table), 10);
   gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, FALSE);
   gtk_widget_show(table);

   create_label_in_table(table, 0, 0, "x:");
   props->x = create_spin_button_in_table(table, 0, 1, 1, 0, max_width - 1);
   gtk_signal_connect(GTK_OBJECT(props->x), "changed", 
		      (GtkSignalFunc) x_changed_cb, (gpointer) props);
   gtk_widget_set_usize(props->x, 64, -1);
   create_label_in_table(table, 0, 2, _("pixels"));

   create_label_in_table(table, 1, 0, "y:");
   props->y = create_spin_button_in_table(table, 1, 1, 1, 0, max_height - 1);
   gtk_signal_connect(GTK_OBJECT(props->y), "changed", 
		      (GtkSignalFunc) y_changed_cb, (gpointer) props);
   gtk_widget_set_usize(props->y, 64, -1);
   create_label_in_table(table, 1, 2, _("pixels"));

   props->update = gtk_button_new_with_label(_("Update"));
   gtk_signal_connect(GTK_OBJECT(props->update), "clicked",
		      GTK_SIGNAL_FUNC(update_button_clicked), props);
   gtk_table_attach_defaults(GTK_TABLE(table), props->update, 1, 2, 2, 3);
   gtk_widget_show(props->update);

   props->insert = gtk_button_new_with_label(_("Insert"));
   gtk_signal_connect(GTK_OBJECT(props->insert), "clicked",
		      GTK_SIGNAL_FUNC(insert_button_clicked), props);
   gtk_table_attach_defaults(GTK_TABLE(table), props->insert, 1, 2, 3, 4);
   gtk_widget_show(props->insert);

   props->append = gtk_button_new_with_label(_("Append"));
   gtk_signal_connect(GTK_OBJECT(props->append), "clicked",
		      GTK_SIGNAL_FUNC(append_button_clicked), props);
   gtk_table_attach_defaults(GTK_TABLE(table), props->append, 1, 2, 4, 5);
   gtk_widget_show(props->append);

   props->remove = gtk_button_new_with_label(_("Remove"));
   gtk_signal_connect(GTK_OBJECT(props->remove), "clicked",
		      GTK_SIGNAL_FUNC(remove_button_clicked), props);
   gtk_table_attach_defaults(GTK_TABLE(table), props->remove, 1, 2, 5, 6);
   gtk_widget_show(props->remove);

   props->timeout = 0;

   return props;
}

static gint
update_timeout(gpointer data)
{
   PolygonProperties_t *props = (PolygonProperties_t*) data;
   polygon_fill_info_tab(props->obj, data);
   return FALSE;
}

static void
polygon_update_info_widget(Object_t *obj, gpointer data)
{
   PolygonProperties_t *props = (PolygonProperties_t*) data;
   
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->x), _sash_point->x);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->y), _sash_point->y);
   
   if (props->selected_row != _sash_index) {
      props->selected_row = _sash_index;
      gtk_clist_select_row(GTK_CLIST(props->list), _sash_index, -1);
   }
   if (props->timeout)
      gtk_timeout_remove(props->timeout);
   props->timeout = gtk_timeout_add(1000, update_timeout, data);
}

static void
polygon_fill_info_tab(Object_t *obj, gpointer data)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   PolygonProperties_t *props = (PolygonProperties_t*) data;
   GList     *p;

   props->obj = obj;
   gtk_clist_freeze(GTK_CLIST(props->list));
   gtk_clist_clear(GTK_CLIST(props->list));
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      char x[16], y[16];
      char *text[2];

      text[0] = x;
      text[1] = y;
      sprintf(x, "%d", point->x);
      sprintf(y, "%d", point->y);
      gtk_clist_append(GTK_CLIST(props->list), text);
   }
   gtk_clist_select_row(GTK_CLIST(props->list), _sash_index, -1);
   gtk_clist_thaw(GTK_CLIST(props->list));

   set_buttons_sensitivity(props);
}

static void 
polygon_set_initial_focus(Object_t *obj, gpointer data)
{
   PolygonProperties_t *props = (PolygonProperties_t*) data;
   gtk_widget_grab_focus(props->x);
}

static void 
polygon_update(Object_t* obj, gpointer data)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   PolygonProperties_t *props = (PolygonProperties_t*) data;
   gint rows = GTK_CLIST(props->list)->rows;
   int i;

   g_list_free(polygon->points);
   polygon->points = NULL;

   for (i = 0; i < rows; i++) {
      gchar *text;
      GdkPoint *point = g_new(GdkPoint, 1);

      gtk_clist_get_text(GTK_CLIST(props->list), i, 0, &text);
      point->x = atoi(text);
      gtk_clist_get_text(GTK_CLIST(props->list), i, 1, &text);
      point->y = atoi(text);

      polygon->points = g_list_append(polygon->points, point);
   }
}

static void
polygon_write_csim(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;

   output(param, "\"POLY\" COORDS=\"");
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      output(param, "%d,%d", point->x, point->y);
      output(param, "%c", (p->next) ? ',' : '"');
   }
}

static void
polygon_write_cern(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;
   GdkPoint  *first = (GdkPoint*) polygon->points->data;

   output(param, "poly ");
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      output(param, "(%d,%d) ", point->x, point->y);
   }
   output(param, "(%d,%d)", first->x, first->y);
}

static void
polygon_write_ncsa(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;
   GdkPoint  *first = (GdkPoint*) polygon->points->data;

   output(param, "poly %s", obj->url);
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      output(param, " %d,%d", point->x, point->y);
   }
   output(param, " %d,%d", first->x, first->y);
}

static gint _insert_edge;
static gint _insert_x;
static gint _insert_y;

static void
polygon_insert_point(GtkWidget *widget, gpointer data)
{
   Command_t *command = insert_point_command_new(get_popup_object(), _insert_x,
						 _insert_y, _insert_edge);
   command_execute(command);
}

static void
polygon_delete_point(GtkWidget *widget, gpointer data)
{
   Command_t *command = delete_point_command_new(get_popup_object(), 
						 _sash_point);
   command_execute(command);
}

static gboolean
point_near_edge(GdkPoint *first, GdkPoint *second, gint x, gint y)
{
   gint den, nom;
   gdouble u;

   den = (first->x - x) * (first->x - second->x) +
      (first->y - y) * (first->y - second->y);
   nom = (second->x - first->x) * (second->x - first->x) +
      (second->y - first->y) * (second->y - first->y);
   u = (gdouble) den / nom;
   if (u >= 0.0 && u <= 1.0) {
      gint sx = first->x + (gint) (u * (second->x - first->x)) - x;
      gint sy = first->y + (gint) (u * (second->y - first->y)) - y;
      return sx * sx + sy * sy <= 25; /* Fix me! */
   }
   return 0;
}

static gint
polygon_near_edge(Object_t *obj, gint x, gint y)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p = polygon->points;
   GdkPoint  *first = (GdkPoint*) p->data;
   GdkPoint  *prev = first;
   gint n = 1;

   for (p = p->next; p; p = p->next, n++) {
      GdkPoint *next = (GdkPoint*) p->data;
      if (point_near_edge(prev, next, x, y))
	 return n;
      prev = next;
   }
   return (point_near_edge(prev, first, x, y)) ? n + 1 : 0;
}

static void 
polygon_do_popup(Object_t *obj, GdkEventButton *event)
{
   gint x = get_real_coord((gint) event->x);
   gint y = get_real_coord((gint) event->y);

   if (polygon_near_sash(obj, x, y)) {
      static ObjectPopup_t *delete_popup;
      if (!delete_popup) {
	 delete_popup = make_object_popup();
	 object_popup_prepend_menu(delete_popup, _("Delete Point"), 
				   polygon_delete_point, delete_popup);
      }
      object_handle_popup(delete_popup, obj, event);
   } else {
      _insert_edge = polygon_near_edge(obj, x, y);
      if (_insert_edge) {
	 static ObjectPopup_t *insert_popup;

	 _insert_x = x;
	 _insert_y = y;

	 if (!insert_popup) {
	    insert_popup = make_object_popup();
	    object_popup_prepend_menu(insert_popup, _("Insert Point"), 
				      polygon_insert_point, insert_popup);
	 }
	 object_handle_popup(insert_popup, obj, event);
      } else {
	 object_do_popup(obj, event);
      }
   }
}

static char** 
polygon_get_icon_data(void)
{
   return polygon_xpm;
}

static GList *_prev_link;

static gboolean
polygon_factory_finish(Object_t *obj, gint x, gint y)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GdkPoint *prev_point = (GdkPoint*) _prev_link->data;

   if (x == prev_point->x && y == prev_point->y) {
      polygon_remove_last_point(polygon);
      return TRUE;
   } else {
      g_list_append(polygon->points, new_point(x, y));
      _prev_link = _prev_link->next;
   }
   return FALSE;
}

static gboolean
polygon_factory_cancel(GdkEventButton *event, Object_t *obj)
{
   if (event->state & GDK_SHIFT_MASK) {
      return TRUE;
   } else {
      Polygon_t *polygon = ObjectToPolygon(obj);
      GList *link = _prev_link;
	    
      _prev_link = _prev_link->prev;
      g_free((GdkPoint*) link->data);
      polygon->points = g_list_remove_link(polygon->points, link);
   }
   return _prev_link == NULL;
}

static Object_t*
polygon_factory_create_object(gint x, gint y)
{
   GList *points;

   points = _prev_link = g_list_append(NULL, new_point(x, y));
   points = g_list_append(points, new_point(x, y));

   return create_polygon(points);
}

static void
polygon_factory_set_xy(Object_t *obj, guint state, gint x, gint y)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList *last = g_list_last(polygon->points);
   GdkPoint *point = (GdkPoint*) last->data;
   GdkPoint *prev = (GdkPoint*) last->prev->data;

   point->x = x;
   point->y = y;

   main_set_dimension(x - prev->x, y - prev->y);
}

static ObjectFactory_t polygon_factory = {
   NULL,			/* Object pointer */
   polygon_factory_finish,
   polygon_factory_cancel,
   polygon_factory_create_object,
   polygon_factory_set_xy
};

ObjectFactory_t*
get_polygon_factory(guint state)
{
   return &polygon_factory;
}

void
polygon_remove_last_point(Polygon_t *polygon)
{
   GList *last = g_list_last(polygon->points);
   g_free((GdkPoint*) last->data);
   polygon->points = g_list_remove_link(polygon->points, last);
}

GdkPoint*
new_point(gint x, gint y)
{
   GdkPoint *point = g_new(GdkPoint, 1);
   point->x = x;
   point->y = y;
   return point;
}
