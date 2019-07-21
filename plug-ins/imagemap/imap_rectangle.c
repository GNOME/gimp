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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <stdlib.h> /* abs */

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_icons.h"
#include "imap_main.h"
#include "imap_misc.h"
#include "imap_object_popup.h"
#include "imap_rectangle.h"
#include "imap_ui_grid.h"

#include "libgimp/stdplugins-intl.h"


static gboolean rectangle_is_valid(Object_t *obj);
static Object_t *rectangle_clone(Object_t *obj);
static void rectangle_assign(Object_t *obj, Object_t *des);
static void rectangle_normalize(Object_t *obj);
static void rectangle_draw(Object_t *obj, cairo_t *cr);
static void rectangle_draw_sashes(Object_t *obj, cairo_t *cr);
static MoveSashFunc_t rectangle_near_sash(Object_t *obj, gint x, gint y);
static gboolean rectangle_point_is_on(Object_t *obj, gint x, gint y);
static void rectangle_get_dimensions(Object_t *obj, gint *x, gint *y,
                                     gint *width, gint *height);
static void rectangle_resize(Object_t *obj, gint percentage_x,
                             gint percentage_y);
static void rectangle_move(Object_t *obj, gint dx, gint dy);
static gpointer rectangle_create_info_widget(GtkWidget *frame);
static void rectangle_fill_info_tab(Object_t *obj, gpointer data);
static void rectangle_set_initial_focus(Object_t *obj, gpointer data);
static void rectangle_update(Object_t *obj, gpointer data);
static void rectangle_write_csim(Object_t *obj, gpointer param,
                                 OutputFunc_t output);
static void rectangle_write_cern(Object_t *obj, gpointer param,
                                 OutputFunc_t output);
static void rectangle_write_ncsa(Object_t *obj, gpointer param,
                                 OutputFunc_t output);
static const gchar* rectangle_get_icon_name(void);

static ObjectClass_t rectangle_class = {
   N_("_Rectangle"),
   NULL,                        /* info_dialog */

   rectangle_is_valid,
   NULL,                        /* rectangle_destruct */
   rectangle_clone,
   rectangle_assign,
   rectangle_normalize,
   rectangle_draw,
   rectangle_draw_sashes,
   rectangle_near_sash,
   rectangle_point_is_on,
   rectangle_get_dimensions,
   rectangle_resize,
   rectangle_move,
   rectangle_create_info_widget,
   rectangle_fill_info_tab,     /* rectangle_update_info_widget */
   rectangle_fill_info_tab,
   rectangle_set_initial_focus,
   rectangle_update,
   rectangle_write_csim,
   rectangle_write_cern,
   rectangle_write_ncsa,
   object_do_popup,
   rectangle_get_icon_name
};

Object_t*
create_rectangle(gint x, gint y, gint width, gint height)
{
   Rectangle_t *rectangle = g_new(Rectangle_t, 1);
   rectangle->x = x;
   rectangle->y = y;
   rectangle->width = width;
   rectangle->height = height;
   return object_init(&rectangle->obj, &rectangle_class);
}

static void
draw_any_rectangle(cairo_t *cr, gint x, gint y, gint w, gint h)
{
   if (w < 0) {
      x += w;
      w = -w;
   }
   if (h < 0) {
      y += h;
      h = -h;
   }
   draw_rectangle(cr, FALSE, x, y, w, h);
}

static gboolean
rectangle_is_valid(Object_t *obj)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   return rectangle->width && rectangle->height;
}

static Object_t*
rectangle_clone(Object_t *obj)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   Rectangle_t *clone = g_new(Rectangle_t, 1);

   clone->x = rectangle->x;
   clone->y = rectangle->y;
   clone->width = rectangle->width;
   clone->height = rectangle->height;
   return &clone->obj;
}

static void
rectangle_assign(Object_t *obj, Object_t *des)
{
   Rectangle_t *src_rectangle = ObjectToRectangle(obj);
   Rectangle_t *des_rectangle = ObjectToRectangle(des);
   des_rectangle->x = src_rectangle->x;
   des_rectangle->y = src_rectangle->y;
   des_rectangle->width = src_rectangle->width;
   des_rectangle->height = src_rectangle->height;
}

static void
rectangle_normalize(Object_t *obj)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   if (rectangle->width < 0) {
      rectangle->x += rectangle->width;
      rectangle->width = -rectangle->width;
   }
   if (rectangle->height < 0) {
      rectangle->y += rectangle->height;
      rectangle->height = -rectangle->height;
   }
}

static void
rectangle_draw(Object_t *obj, cairo_t *cr)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   draw_any_rectangle(cr, rectangle->x, rectangle->y,
                      rectangle->width, rectangle->height);
}

static void
rectangle_draw_sashes(Object_t *obj, cairo_t *cr)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   draw_sash(cr, rectangle->x, rectangle->y);
   draw_sash(cr, rectangle->x + rectangle->width / 2, rectangle->y);
   draw_sash(cr, rectangle->x + rectangle->width, rectangle->y);
   draw_sash(cr, rectangle->x, rectangle->y + rectangle->height / 2);
   draw_sash(cr, rectangle->x + rectangle->width,
             rectangle->y + rectangle->height / 2);
   draw_sash(cr, rectangle->x, rectangle->y + rectangle->height);
   draw_sash(cr, rectangle->x + rectangle->width / 2,
             rectangle->y + rectangle->height);
   draw_sash(cr, rectangle->x + rectangle->width,
             rectangle->y + rectangle->height);
}

static void
MoveUpperSash(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->y += dy;
   rectangle->height -= dy;
}

static void
MoveLeftSash(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->x += dx;
   rectangle->width -= dx;
}

static void
MoveRightSash(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->width += dx;
}

static void
MoveLowerSash(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->height += dy;
}

static void
MoveUpperLeftSash(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->x += dx;
   rectangle->y += dy;
   rectangle->width -= dx;
   rectangle->height -= dy;
}

static void
MoveUpperRightSash(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->y += dy;
   rectangle->width += dx;
   rectangle->height -= dy;
}

static void
MoveLowerLeftSash(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->x += dx;
   rectangle->width -= dx;
   rectangle->height += dy;
}

static void
MoveLowerRightSash(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->width += dx;
   rectangle->height += dy;
}

static MoveSashFunc_t
rectangle_near_sash(Object_t *obj, gint x, gint y)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   if (near_sash(rectangle->x, rectangle->y, x, y))
      return MoveUpperLeftSash;
   else if (near_sash(rectangle->x + rectangle->width / 2, rectangle->y, x, y))
      return MoveUpperSash;
   else if (near_sash(rectangle->x + rectangle->width, rectangle->y, x, y))
      return MoveUpperRightSash;
   else if (near_sash(rectangle->x, rectangle->y + rectangle->height / 2,
                      x, y))
      return MoveLeftSash;
   else if (near_sash(rectangle->x + rectangle->width,
                      rectangle->y + rectangle->height / 2, x, y))
      return MoveRightSash;
   else if (near_sash(rectangle->x, rectangle->y + rectangle->height, x, y))
      return MoveLowerLeftSash;
   else if (near_sash(rectangle->x + rectangle->width / 2,
                      rectangle->y + rectangle->height, x, y))
      return MoveLowerSash;
   else if (near_sash(rectangle->x + rectangle->width,
                      rectangle->y + rectangle->height, x, y))
      return MoveLowerRightSash;
   return NULL;
}

static gboolean
rectangle_point_is_on(Object_t *obj, gint x, gint y)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   return x >= rectangle->x && x <= rectangle->x + rectangle->width &&
      y >= rectangle->y && y <= rectangle->y + rectangle->height;
}

static void
rectangle_get_dimensions(Object_t *obj, gint *x, gint *y,
                         gint *width, gint *height)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   *x = rectangle->x;
   *y = rectangle->y;
   *width = rectangle->width;
   *height = rectangle->height;
}

static void
rectangle_resize(Object_t *obj, gint percentage_x, gint percentage_y)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->x = rectangle->x * percentage_x / 100;
   rectangle->y = rectangle->y * percentage_y / 100;
   rectangle->width = rectangle->width * percentage_x / 100;
   rectangle->height = rectangle->height * percentage_y / 100;
}

static void
rectangle_move(Object_t *obj, gint dx, gint dy)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   rectangle->x += dx;
   rectangle->y += dy;
}

typedef struct {
   Object_t  *obj;
   GtkWidget *x;
   GtkWidget *y;
   GtkWidget *width;
   GtkWidget *height;
   GtkWidget *chain_button;
} RectangleProperties_t;

static void
x_changed_cb(GtkWidget *widget, gpointer data)
{
   RectangleProperties_t *props = (RectangleProperties_t*) data;
   Object_t *obj = props->obj;
   gint x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

   if (gimp_chain_button_get_active(GIMP_CHAIN_BUTTON(props->chain_button)))
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->y), x);

   ObjectToRectangle(obj)->x = x;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static void
y_changed_cb(GtkWidget *widget, gpointer data)
{
   Object_t *obj = ((RectangleProperties_t*) data)->obj;
   gint y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   ObjectToRectangle(obj)->y = y;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static void
width_changed_cb(GtkWidget *widget, gpointer data)
{
   Object_t *obj = ((RectangleProperties_t*) data)->obj;
   gint width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   ObjectToRectangle(obj)->width = width;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static void
height_changed_cb(GtkWidget *widget, gpointer data)
{
   Object_t *obj = ((RectangleProperties_t*) data)->obj;
   gint height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   ObjectToRectangle(obj)->height = height;
   edit_area_info_dialog_emit_geometry_signal(obj->class->info_dialog);
}

static gpointer
rectangle_create_info_widget(GtkWidget *frame)
{
   RectangleProperties_t *props = g_new(RectangleProperties_t, 1);
   GtkWidget *grid, *label, *chain_button;
   gint max_width = get_image_width();
   gint max_height = get_image_height();

   grid = gtk_grid_new ();
   gtk_container_add (GTK_CONTAINER (frame), grid);
   gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
   gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
   gtk_widget_show(grid);

   label = create_label_in_grid (grid, 0, 0, _("Upper left _x:"));
   props->x = create_spin_button_in_grid (grid, label, 0, 1, 1, 0,
                                          max_width - 1);
   g_signal_connect(props->x, "value-changed",
                    G_CALLBACK(x_changed_cb), (gpointer) props);
   create_label_in_grid (grid, 0, 3, _("pixels"));

   label = create_label_in_grid (grid, 1, 0, _("Upper left _y:"));
   props->y = create_spin_button_in_grid (grid, label, 1, 1, 1, 0,
                                          max_height - 1);
   g_signal_connect(props->y, "value-changed",
                    G_CALLBACK(y_changed_cb), (gpointer) props);
   create_label_in_grid (grid, 1, 3, _("pixels"));

   label = create_label_in_grid (grid, 2, 0, _("_Width:"));
   props->width = create_spin_button_in_grid (grid, label, 2, 1, 1, 1,
                                              max_width);
   g_signal_connect(props->width, "value-changed",
                    G_CALLBACK(width_changed_cb), (gpointer) props);
   create_label_in_grid (grid, 2, 3, _("pixels"));

   label = create_label_in_grid (grid, 3, 0, _("_Height:"));
   props->height = create_spin_button_in_grid (grid, label, 3, 1, 1, 1,
                                               max_height);
   g_signal_connect(props->height, "value-changed",
                    G_CALLBACK(height_changed_cb), (gpointer) props);
   create_label_in_grid (grid, 3, 3, _("pixels"));

   chain_button = gimp_chain_button_new(GIMP_CHAIN_RIGHT);
   props->chain_button = chain_button;
   gtk_grid_attach (GTK_GRID (grid), chain_button, 2, 2, 1, 2);
   gtk_widget_show(chain_button);

   return props;
}

static void
rectangle_fill_info_tab(Object_t *obj, gpointer data)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   RectangleProperties_t *props = (RectangleProperties_t*) data;

   props->obj = obj;
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->x), rectangle->x);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->y), rectangle->y);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->width), rectangle->width);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(props->height),
                             rectangle->height);
}

static void
rectangle_set_initial_focus(Object_t *obj, gpointer data)
{
   RectangleProperties_t *props = (RectangleProperties_t*) data;
   gtk_widget_grab_focus(props->x);
}

static void
rectangle_update(Object_t* obj, gpointer data)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   RectangleProperties_t *props = (RectangleProperties_t*) data;

   rectangle->x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(props->x));
   rectangle->y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(props->y));
   rectangle->width = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(props->width));
   rectangle->height = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(props->height));
}

static void
rectangle_write_csim(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   output(param, "\"rect\" coords=\"%d,%d,%d,%d\"", rectangle->x, rectangle->y,
          rectangle->x + rectangle->width, rectangle->y + rectangle->height);
}

static void
rectangle_write_cern(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   output(param, "rect (%d,%d) (%d,%d)", rectangle->x, rectangle->y,
          rectangle->x + rectangle->width, rectangle->y + rectangle->height);
}

static void
rectangle_write_ncsa(Object_t *obj, gpointer param, OutputFunc_t output)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);
   output(param, "rect %s %d,%d %d,%d", obj->url,
          rectangle->x, rectangle->y,
          rectangle->x + rectangle->width, rectangle->y + rectangle->height);
}

static const gchar*
rectangle_get_icon_name(void)
{
   return IMAP_RECTANGLE;
}

static gboolean
rectangle_factory_finish(Object_t *obj, gint x, gint y)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);

   rectangle->width = x - rectangle->x;
   rectangle->height = y - rectangle->y;

   rectangle_normalize(obj);

   return TRUE;
}

static Object_t*
rectangle_factory_create_object(gint x, gint y)
{
   return create_rectangle(x, y, 0, 0);
}

static void
rectangle_factory_set_xy(Object_t *obj, guint state, gint x, gint y)
{
   Rectangle_t *rectangle = ObjectToRectangle(obj);

   rectangle->width = x - rectangle->x;
   rectangle->height = y - rectangle->y;

   if (state & GDK_SHIFT_MASK){
      gint width = abs(rectangle->width);
      gint height = abs(rectangle->height);
      if (width < height)
         rectangle->height = (rectangle->height < 0) ? -width : width;
      else
         rectangle->width = (rectangle->width < 0) ? -height : height;
   }

   main_set_dimension(rectangle->width, rectangle->height);
}

static ObjectFactory_t rectangle_factory = {
   NULL,                        /* Object pointer */
   rectangle_factory_finish,
   NULL,                        /* Cancel func */
   rectangle_factory_create_object,
   rectangle_factory_set_xy
};

ObjectFactory_t*
get_rectangle_factory(guint state)
{
   return &rectangle_factory;
}
