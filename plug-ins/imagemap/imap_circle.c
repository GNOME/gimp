/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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

#include <math.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "imap_circle.h"
#include "imap_main.h"
#include "imap_misc.h"
#include "imap_object_popup.h"
#include "imap_stock.h"
#include "imap_ui_grid.h"

#include "libgimp/stdplugins-intl.h"

static gboolean circle_is_valid(Object_t *obj);
static Object_t *circle_clone(Object_t *obj);
static void circle_assign(Object_t *obj, Object_t *des);
static void circle_draw(Object_t* obj, cairo_t *cr);
static void circle_draw_sashes(Object_t* obj, cairo_t *cr);
static MoveSashFunc_t circle_near_sash(Object_t *obj, gint x, gint y);
static gboolean circle_point_is_on(Object_t *obj, gint x, gint y);
static void circle_get_dimensions(Object_t *obj, gint *x, gint *y,
                                  gint *width, gint *height);
static void circle_resize(Object_t *obj, gint percentage_x, gint percentage_y);
static void circle_move(Object_t *obj, gint dx, gint dy);
static gpointer circle_create_info_widget(GtkWidget *frame);
static void circle_fill_info_tab(Object_t *obj, gpointer data);
static void circle_set_initial_focus(Object_t *obj, gpointer data);
static void circle_update(Object_t* obj, gpointer data);
static void circle_write_csim(Object_t* obj, gpointer param,
                              OutputFunc_t output);
static void circle_write_cern(Object_t* obj, gpointer param,
                              OutputFunc_t output);
static void circle_write_ncsa(Object_t* obj, gpointer param,
                              OutputFunc_t output);
static const gchar* circle_get_stock_icon_name(void);

static ObjectClass_t circle_class = {
   N_("C_ircle"),
   NULL,                        /* info_dialog */

   circle_is_valid,
   NULL,                        /* circle_destruct */
   circle_clone,
   circle_assign,
   NULL,                        /* circle_normalize */
   circle_draw,
   circle_draw_sashes,
   circle_near_sash,
   circle_point_is_on,
   circle_get_dimensions,
   circle_resize,
   circle_move,
   circle_create_info_widget,
   circle_fill_info_tab,        /* circle_update_info_widget */
   circle_fill_info_tab,
   circle_set_initial_focus,
   circle_update,
   circle_write_csim,
   circle_write_cern,
   circle_write_ncsa,
   object_do_popup,
   circle_get_stock_icon_name
};

Object_t*
create_circle(gint x, gint y, gint r)
{
   Circle_t *circle = g_new(Circle_t, 1);
   circle->x = x;
   circle->y = y;
   circle->r = r;
   return object_init(&circle->obj, &circle_class);
}

static gboolean
circle_is_valid(Object_t *obj)
{
   return ObjectToCircle(obj)->r > 0;
}

static Object_t*
circle_clone(Object_t *obj)
{
   Circle_t *circle = ObjectToCircle(obj);
   Circle_t *clone = g_new(Circle_t, 1);

   clone->x = circle->x;
   clone->y = circle->y;
   clone->r = circle->r;
   return &clone->obj;
}

static void
circle_assign(Object_t *obj, Object_t *des)
{
   Circle_t *src_circle = ObjectToCircle(obj);
   Circle_t *des_circle = ObjectToCircle(des);
   des_circle->x = src_circle->x;
   des_circle->y = src_circle->y;
   des_circle->r = src_circle->r;
}

static void
circle_draw(Object_t *obj, cairo_t *cr)
{
   Circle_t *circle = ObjectToCircle(obj);
   draw_circle(cr, circle->x, circle->y, circle->r);
}

static void
circle_draw_sashes(Object_t *obj, cairo_t *cr)
{
   Circle_t *circle = ObjectToCircle(obj);
   draw_sash(cr, circle->x - circle->r, circle->y - circle->r);
   draw_sash(cr, circle->x + circle->r, circle->y - circle->r);
   draw_sash(cr, circle->x - circle->r, circle->y + circle->r);
   draw_sash(cr, circle->x + circle->r, circle->y + circle->r);
}

static gint sash_x;
static gint sash_y;

static void
move_sash(Object_t *obj, gint dx, gint dy)
{
   Circle_t *circle = ObjectToCircle(obj);
   gint rx, ry;
   sash_x += dx;
   sash_y += dy;

   rx = abs(circle->x - sash_x);
   ry = abs(circle->y - sash_y);
   circle->r = (rx > ry) ? rx : ry;
}

static void
circle_resize(Object_t *obj, gint percentage_x, gint percentage_y)
{
   Circle_t *circle = ObjectToCircle(obj);
   circle->x = circle->x * percentage_x / 100;
   circle->y = circle->y * percentage_y / 100;
   circle->r = circle->r * ((percentage_x < percentage_y)
                            ? percentage_x : percentage_y) / 100;
}

static MoveSashFunc_t
circle_near_sash(Object_t *obj, gint x, gint y)
{
   Circle_t *circle = ObjectToCircle(obj);
   sash_x = x;
   sash_y = y;
   if (near_sash(circle->x - circle->r, circle->y - circle->r, x, y) ||
       near_sash(circle->x + circle->r, circle->y - circle->r, x, y) ||
       near_sash(circle->x - circle->r, circle->y + circle->r, x, y) ||
       near_sash(circle->x + circle->r, circle->y + circle->r, x, y))
      return move_sash;
   return NULL;
}

static gboolean
circle_point_is_on(Object_t *obj, gint x, gint y)
{
   Circle_t *circle = ObjectToCircle(obj);
   x -= circle->x;
   y -= circle->y;
   return x * x + y * y <= circle->r * circle->r;
}

static void
circle_get_dimensions(Object_t *obj, gint *x, gint *y,
                      gint *width, gint *height)
{
   Circle_t *circle = ObjectToCircle(obj);
   *x = circle->x - circle->r;
   *y = circle->y - circle->r;
   *width = *height = 2 * circle->r;
}

static void
circle_move(Object_t *obj, gint dx, gint dy)
{
   Circle_t *circle = ObjectToCircle(obj);
   circle->x += dx;
   circle->y += dy;
}

typedef struct {
   Object_t  *obj;
   GtkWidget *x;
   GtkWidget *y;
   GtkWidget *r;
} CircleProperties_t;

static void
x_changed_cb(GtkWidget *widget, gpointer data)
{
   Object_t *obj = ((CircleProperties_t*) data)->obj;
   gint x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   ObjectToCircle(obj)->x = x;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static void
y_changed_cb(GtkWidget *widget, gpointer data)
{
   Object_t *obj = ((CircleProperties_t*) data)->obj;
   gint y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   ObjectToCircle(obj)->y = y;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static void
r_changed_cb(GtkWidget *widget, gpointer data)
{
   Object_t *obj = ((CircleProperties_t*) data)->obj;
   gint r = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   ObjectToCircle(obj)->r = r;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static gpointer
circle_create_info_widget(GtkWidget *frame)
{
   CircleProperties_t *props = g_new(CircleProperties_t, 1);
   GtkWidget *grid, *label;
   gint max_width = get_image_width();
   gint max_height = get_image_height();

   grid = gtk_grid_new ();
   gtk_container_add (GTK_CONTAINER(frame), grid);

   gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
   gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
   gtk_widget_show (grid);

   label = create_label_in_grid (grid, 0, 0, _("Center _x:"));
   props->x = create_spin_button_in_grid (grid, label, 0, 1, 1, 0,
                                          max_width - 1);
   g_signal_connect(props->x, "value-changed",
                    G_CALLBACK (x_changed_cb), (gpointer) props);
   create_label_in_grid (grid, 0, 2, _("pixels"));

   label = create_label_in_grid (grid, 1, 0, _("Center _y:"));
   props->y = create_spin_button_in_grid (grid, label, 1, 1, 1, 0,
                                          max_height - 1);
   g_signal_connect(props->y, "value-changed",
                    G_CALLBACK (y_changed_cb), (gpointer) props);
   create_label_in_grid (grid, 1, 2, _("pixels"));

   label = create_label_in_grid (grid, 2, 0, _("_Radius:"));
   props->r = create_spin_button_in_grid (grid, label, 2, 1, 1, 1, G_MAXINT);
   g_signal_connect(props->r, "value-changed",
                    G_CALLBACK (r_changed_cb), (gpointer) props);
   create_label_in_grid (grid, 2, 2, _("pixels"));

   return props;
}

static void
circle_fill_info_tab(Object_t *obj, gpointer data)
{
   Circle_t *circle = ObjectToCircle(obj);
   CircleProperties_t *props = (CircleProperties_t*) data;

   props->obj = obj;
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->x), circle->x);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->y), circle->y);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->r), circle->r);
}

static void
circle_set_initial_focus(Object_t *obj, gpointer data)
{
   CircleProperties_t *props = (CircleProperties_t*) data;
   gtk_widget_grab_focus(props->x);
}

static void
circle_update(Object_t* obj, gpointer data)
{
   Circle_t *circle = ObjectToCircle(obj);
   CircleProperties_t *props = (CircleProperties_t*) data;

   circle->x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(props->x));
   circle->y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(props->y));
   circle->r = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(props->r));
}

static void
circle_write_csim(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Circle_t *circle = ObjectToCircle(obj);
   output(param, "\"circle\" coords=\"%d,%d,%d\"", circle->x, circle->y,
          circle->r);
}

static void
circle_write_cern(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Circle_t *circle = ObjectToCircle(obj);
   output(param, "circ (%d,%d) %d", circle->x, circle->y, circle->r);
}

static void
circle_write_ncsa(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Circle_t *circle = ObjectToCircle(obj);
   output(param, "circle %s %d,%d %d,%d", obj->url,
          circle->x, circle->y, circle->x, circle->y + circle->r);
}

static const gchar*
circle_get_stock_icon_name(void)
{
   return IMAP_STOCK_CIRCLE;
}

static gint _start_x, _start_y;

static Object_t*
circle_factory_create_object1(gint x, gint y)
{
   _start_x = x;
   _start_y = y;
   return create_circle(x, y, 0);
}

static void
circle_factory_set_xy1(Object_t *obj, guint state, gint x, gint y)
{
   Circle_t *circle = ObjectToCircle(obj);

   circle->x = (_start_x + x) / 2;
   circle->y = (_start_y + y) / 2;
   x -= _start_x;
   y -= _start_y;
   circle->r = (gint) sqrt(x * x + y * y) / 2;

   main_set_dimension(circle->r, circle->r);
}

static ObjectFactory_t circle_factory1 = {
   NULL,                        /* Object pointer */
   NULL,                        /* Finish func */
   NULL,                        /* Cancel func */
   circle_factory_create_object1,
   circle_factory_set_xy1
};

static Object_t*
circle_factory_create_object2(gint x, gint y)
{
   return create_circle(x, y, 0);
}

static void
circle_factory_set_xy2(Object_t *obj, guint state, gint x, gint y)
{
   Circle_t *circle = ObjectToCircle(obj);

   x -= circle->x;
   y -= circle->y;
   circle->r = (gint) sqrt(x * x + y * y);

   main_set_dimension(circle->r, circle->r);
}

static ObjectFactory_t circle_factory2 = {
   NULL,                        /* Object pointer */
   NULL,                        /* Finish func */
   NULL,                        /* Cancel func */
   circle_factory_create_object2,
   circle_factory_set_xy2
};

ObjectFactory_t*
get_circle_factory(guint state)
{
   return (state & GDK_SHIFT_MASK) ? &circle_factory1 : &circle_factory2;
}
