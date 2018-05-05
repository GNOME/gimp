/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "imap_commands.h"
#include "imap_main.h"
#include "imap_misc.h"
#include "imap_menu.h"
#include "imap_object_popup.h"
#include "imap_polygon.h"
#include "imap_stock.h"
#include "imap_ui_grid.h"

#include "libgimp/stdplugins-intl.h"

#define MAX_POLYGON_POINTS 99

static gboolean polygon_is_valid(Object_t *obj);
static void polygon_destruct(Object_t *obj);
static Object_t *polygon_clone(Object_t *obj);
static void polygon_assign(Object_t *obj, Object_t *des);
static void polygon_draw(Object_t* obj, cairo_t *cr);
static void polygon_draw_sashes(Object_t* obj, cairo_t *cr);
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
static const gchar* polygon_get_stock_icon_name(void);

static ObjectClass_t polygon_class = {
   N_("_Polygon"),
   NULL,                        /* info_dialog */

   polygon_is_valid,
   polygon_destruct,
   polygon_clone,
   polygon_assign,
   NULL,                        /* polygon_normalize */
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
   polygon_get_stock_icon_name
};

Object_t*
create_polygon(GList *points)
{
   Polygon_t *polygon = g_new(Polygon_t, 1);
   polygon->points = points;
   return object_init(&polygon->obj, &polygon_class);
}

static void
polygon_free_list (Polygon_t *polygon)
{
  g_list_free_full (polygon->points, (GDestroyNotify) g_free);
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
      polygon_append_point(clone, point->x, point->y);
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
      polygon_append_point(des_polygon, point->x, point->y);
   }
}

static void
polygon_draw(Object_t *obj, cairo_t *cr)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   draw_polygon(cr, polygon->points);
}

static void
polygon_draw_sashes(Object_t *obj, cairo_t *cr)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;
   for (p = polygon->points; p; p = p->next) {
      GdkPoint *point = (GdkPoint*) p->data;
      draw_sash(cr, point->x, point->y);
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
   GtkListStore *store;
   GtkTreeSelection *selection;
   GtkWidget *x;
   GtkWidget *y;
   GtkWidget *update;
   GtkWidget *insert;
   GtkWidget *append;
   GtkWidget *remove;
   gint       selected_row;
   guint      timeout;
} PolygonProperties_t;

static void
select_row_cb(GtkTreeSelection *selection, PolygonProperties_t *data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    GdkPoint *point;

    gtk_tree_model_get (model, &iter, 0, &point, -1);

    _sash_point = point;

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->x), point->x);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->y), point->y);
  }
}

static void
update_button_clicked(GtkWidget *widget, PolygonProperties_t *data)
{
   GtkTreeIter iter;
   GtkTreeModel *model = GTK_TREE_MODEL(data->store);

   if (gtk_tree_selection_get_selected (data->selection, &model, &iter)) {
     GdkPoint *point;
     gint x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->x));
     gint y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->y));

     gtk_tree_model_get (model, &iter, 0, &point, -1);
     point->x = x;
     point->y = y;
     gtk_list_store_set (data->store, &iter, 0, point, -1);
   }
}

static void
set_buttons_sensitivity(PolygonProperties_t *data)
{
   gint rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(data->store),
                                               NULL);
   gtk_widget_set_sensitive(data->insert, rows != MAX_POLYGON_POINTS);
   gtk_widget_set_sensitive(data->append, rows != MAX_POLYGON_POINTS);
   gtk_widget_set_sensitive(data->remove, rows > 2);
}

static void
insert_button_clicked(GtkWidget *widget, PolygonProperties_t *data)
{
   GtkTreeIter iter;
   GtkTreeModel *model = GTK_TREE_MODEL(data->store);

   if (gtk_tree_selection_get_selected (data->selection, &model, &iter)) {
     Polygon_t *polygon = ObjectToPolygon(data->obj);
     GdkPoint *point;
     GList *here;
     gint x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->x));
     gint y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->y));

     gtk_tree_model_get (model, &iter, 0, &point, -1);
     here = g_list_find(polygon->points, point);
     polygon->points = g_list_insert_before(polygon->points, here,
                                            new_point(x, y));
     polygon_fill_info_tab(data->obj, data);
   }
}

static void
append_button_clicked(GtkWidget *widget, PolygonProperties_t *data)
{
   gint x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->x));
   gint y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->y));

   polygon_append_point(ObjectToPolygon(data->obj), x, y);
   polygon_fill_info_tab(data->obj, data);
}

static void
remove_button_clicked(GtkWidget *widget, PolygonProperties_t *data)
{
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(data->store);

  if (gtk_tree_selection_get_selected (data->selection, &model, &iter)) {
    Polygon_t *polygon = ObjectToPolygon(data->obj);
    GdkPoint *point;

    gtk_tree_model_get (model, &iter, 0, &point, -1);
    polygon->points = g_list_remove(polygon->points, point);
    g_free(point);

    polygon_fill_info_tab(data->obj, data);
  }
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

static void
render_x(GtkTreeViewColumn *column, GtkCellRenderer *cell,
         GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
  GdkPoint *point;
  gchar scratch[16];

  gtk_tree_model_get(tree_model, iter, 0, &point, -1);
  sprintf(scratch, "%d", point->x);
  g_object_set(cell, "text", scratch, "xalign", 1.0, NULL);
}

static void
render_y(GtkTreeViewColumn *column, GtkCellRenderer *cell,
         GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
  GdkPoint *point;
  gchar scratch[16];

  gtk_tree_model_get(tree_model, iter, 0, &point, -1);
  sprintf(scratch, "%d", point->y);
  g_object_set(cell, "text", scratch, "xalign", 1.0, NULL);
}

static gpointer
polygon_create_info_widget(GtkWidget *frame)
{
   PolygonProperties_t *props = g_new(PolygonProperties_t, 1);
   GtkWidget *hbox, *swin, *grid, *label;
   GtkWidget *view;
   gint max_width = get_image_width();
   gint max_height = get_image_height();
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;

   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
   gtk_container_add(GTK_CONTAINER(frame), hbox);
   gtk_widget_show(hbox);

   swin = gtk_scrolled_window_new(NULL, NULL);

   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(swin),
                                        GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(hbox), swin, FALSE, FALSE, FALSE);
   gtk_widget_show(swin);

   props->store = gtk_list_store_new (1, G_TYPE_POINTER);
   view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (props->store));
   gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
   g_object_unref (props->store);
   gtk_widget_show (view);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes (_("x (pixels)"),
                                                      renderer,
                                                      NULL);
   gtk_tree_view_column_set_cell_data_func(column, renderer,
                                           render_x, props, NULL);
   gtk_tree_view_column_set_alignment(column, 0.5);
   gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes (_("y (pixels)"),
                                                      renderer,
                                                      NULL);
   gtk_tree_view_column_set_cell_data_func(column, renderer,
                                           render_y, props, NULL);
   gtk_tree_view_column_set_alignment(column, 0.5);
   gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

   gtk_container_add (GTK_CONTAINER (swin), view);

   grid = gtk_grid_new ();
   gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
   gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
   gtk_box_pack_start (GTK_BOX(hbox), grid, FALSE, FALSE, FALSE);
   gtk_widget_show (grid);

   label = create_label_in_grid (grid, 0, 0, "_x:");
   props->x = create_spin_button_in_grid (grid, label, 0, 1, 1, 0,
                                          max_width - 1);
   g_signal_connect(props->x, "changed",
                    G_CALLBACK(x_changed_cb), (gpointer) props);
   gtk_widget_set_size_request(props->x, 64, -1);
   create_label_in_grid (grid, 0, 2, _("pixels"));

   label = create_label_in_grid (grid, 1, 0, "_y:");
   props->y = create_spin_button_in_grid (grid, label, 1, 1, 1, 0,
                                          max_height - 1);
   g_signal_connect(props->y, "changed",
                    G_CALLBACK(y_changed_cb), (gpointer) props);
   gtk_widget_set_size_request(props->y, 64, -1);
   create_label_in_grid (grid, 1, 2, _("pixels"));

   props->update = gtk_button_new_with_mnemonic(_("_Update"));
   g_signal_connect(props->update, "clicked",
                    G_CALLBACK(update_button_clicked), props);
   gtk_grid_attach (GTK_GRID (grid), props->update, 1, 2, 1, 1);
   gtk_widget_show(props->update);

   props->insert = gtk_button_new_with_mnemonic(_("_Insert"));
   g_signal_connect(props->insert, "clicked",
                    G_CALLBACK(insert_button_clicked), props);
   gtk_grid_attach (GTK_GRID (grid), props->insert, 1, 3, 1, 1);
   gtk_widget_show(props->insert);

   props->append = gtk_button_new_with_mnemonic(_("A_ppend"));
   g_signal_connect(props->append, "clicked",
                    G_CALLBACK(append_button_clicked), props);
   gtk_grid_attach (GTK_GRID (grid), props->append, 1, 4, 1, 1);
   gtk_widget_show(props->append);

   props->remove = gtk_button_new_with_mnemonic(_("_Remove"));
   g_signal_connect(props->remove, "clicked",
                    G_CALLBACK(remove_button_clicked), props);
   gtk_grid_attach (GTK_GRID (grid), props->remove, 1, 5, 1, 1);
   gtk_widget_show(props->remove);

   props->timeout = 0;

   props->selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (view));
   gtk_tree_selection_set_mode(props->selection, GTK_SELECTION_SINGLE);
   g_signal_connect (props->selection, "changed",
                     G_CALLBACK (select_row_cb), props);

   return props;
}

static gboolean
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
   GtkTreeIter iter;

   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->x), _sash_point->x);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->y), _sash_point->y);

   if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(props->store), &iter,
                                     NULL, _sash_index)) {
     gtk_tree_selection_select_iter(props->selection, &iter);
   }

   if (props->timeout)
      g_source_remove(props->timeout);
   props->timeout = g_timeout_add(1000, update_timeout, data);
}

static void
polygon_fill_info_tab(Object_t *obj, gpointer data)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   PolygonProperties_t *props = (PolygonProperties_t*) data;
   GtkTreeIter iter;
   GList     *p;

   props->obj = obj;

   gtk_list_store_clear(props->store);

   for (p = polygon->points; p; p = p->next) {
     gtk_list_store_append(props->store, &iter);
     gtk_list_store_set(props->store, &iter, 0, p->data, -1);
   }

   if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(props->store), &iter,
                                     NULL, _sash_index)) {
     gtk_tree_selection_select_iter(props->selection, &iter);
   }
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
  /* Nothing to be done! */
}

static void
polygon_write_csim(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Polygon_t *polygon = ObjectToPolygon(obj);
   GList     *p;

   output(param, "\"poly\" coords=\"");
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

static Object_t *_current_obj;
static gboolean _insert_edge;
static gint _insert_x;
static gint _insert_y;

void
polygon_insert_point(void)
{
  Command_t *command = insert_point_command_new (_current_obj, _insert_x,
                                                 _insert_y, _insert_edge);
  command_execute (command);
}

void
polygon_delete_point(void)
{
  Command_t *command = delete_point_command_new(_current_obj, _sash_point);
  command_execute (command);
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
   return FALSE;
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
polygon_handle_popup (GdkEventButton *event, gboolean near_sash,
                      gboolean near_edge)
{
  GtkWidget *popup = menu_get_widget ("/PolygonPopupMenu");
  GtkWidget *delete = menu_get_widget ("/PolygonPopupMenu/DeletePoint");
  GtkWidget *insert = menu_get_widget ("/PolygonPopupMenu/InsertPoint");

  gtk_widget_set_sensitive (delete, near_sash);
  gtk_widget_set_sensitive (insert, near_edge);

  gtk_menu_popup_at_pointer (GTK_MENU (popup), (GdkEvent *) event);
}

static void
polygon_do_popup(Object_t *obj, GdkEventButton *event)
{
  gint x = get_real_coord ((gint) event->x);
  gint y = get_real_coord ((gint) event->y);

  _current_obj = obj;

  if (polygon_near_sash (obj, x, y))
    {
      polygon_handle_popup (event, TRUE, FALSE);
    }
  else
    {
      _insert_edge = polygon_near_edge (obj, x, y);
      if (_insert_edge)
        {
          _insert_x = x;
           _insert_y = y;

           polygon_handle_popup (event, FALSE, TRUE);
        }
      else {
        object_do_popup (obj, event);
      }
    }
}

static const gchar*
polygon_get_stock_icon_name(void)
{
   return IMAP_STOCK_POLYGON;
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
      polygon_append_point(polygon, x, y);
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
   NULL,                        /* Object pointer */
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

void
polygon_append_point(Polygon_t *polygon, gint x, gint y)
{
  polygon->points = g_list_append(polygon->points, new_point(x, y));
}
