/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpoffsetarea.c
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpwidgetsmarshal.h"
#include "gimpoffsetarea.h"


/**
 * SECTION: gimpoffsetarea
 * @title: GimpOffsetArea
 * @short_description: Widget to control image offsets.
 *
 * Widget to control image offsets.
 **/


#define DRAWING_AREA_SIZE 200


enum
{
  OFFSETS_CHANGED,
  LAST_SIGNAL
};


typedef struct _GimpOffsetAreaPrivate GimpOffsetAreaPrivate;

struct _GimpOffsetAreaPrivate
{
  gint    orig_width;
  gint    orig_height;
  gint    width;
  gint    height;
  gint    offset_x;
  gint    offset_y;
  gdouble display_ratio_x;
  gdouble display_ratio_y;
};

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                                      GIMP_TYPE_OFFSET_AREA, \
                                                      GimpOffsetAreaPrivate)


static void      gimp_offset_area_resize        (GimpOffsetArea *area);

static void      gimp_offset_area_realize       (GtkWidget      *widget);
static void      gimp_offset_area_size_allocate (GtkWidget      *widget,
                                                 GtkAllocation  *allocation);
static gboolean  gimp_offset_area_event         (GtkWidget      *widget,
                                                 GdkEvent       *event);
static gboolean  gimp_offset_area_draw          (GtkWidget      *widget,
                                                 cairo_t        *cr);


G_DEFINE_TYPE (GimpOffsetArea, gimp_offset_area, GTK_TYPE_DRAWING_AREA)

#define parent_class gimp_offset_area_parent_class

static guint gimp_offset_area_signals[LAST_SIGNAL] = { 0 };


static void
gimp_offset_area_class_init (GimpOffsetAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gimp_offset_area_signals[OFFSETS_CHANGED] =
    g_signal_new ("offsets-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpOffsetAreaClass, offsets_changed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  widget_class->size_allocate = gimp_offset_area_size_allocate;
  widget_class->realize       = gimp_offset_area_realize;
  widget_class->event         = gimp_offset_area_event;
  widget_class->draw          = gimp_offset_area_draw;

  g_type_class_add_private (object_class, sizeof (GimpOffsetAreaPrivate));
}

static void
gimp_offset_area_init (GimpOffsetArea *area)
{
  GimpOffsetAreaPrivate *private = GET_PRIVATE (area);

  private->orig_width      = 0;
  private->orig_height     = 0;
  private->width           = 0;
  private->height          = 0;
  private->offset_x        = 0;
  private->offset_y        = 0;
  private->display_ratio_x = 1.0;
  private->display_ratio_y = 1.0;

  gtk_widget_add_events (GTK_WIDGET (area),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK);
}

/**
 * gimp_offset_area_new:
 * @orig_width: the original width
 * @orig_height: the original height
 *
 * Creates a new #GimpOffsetArea widget. A #GimpOffsetArea can be used
 * when resizing an image or a drawable to allow the user to interactively
 * specify the new offsets.
 *
 * Return value: the new #GimpOffsetArea widget.
 **/
GtkWidget *
gimp_offset_area_new (gint orig_width,
                      gint orig_height)
{
  GimpOffsetArea        *area;
  GimpOffsetAreaPrivate *private;

  g_return_val_if_fail (orig_width  > 0, NULL);
  g_return_val_if_fail (orig_height > 0, NULL);

  area = g_object_new (GIMP_TYPE_OFFSET_AREA, NULL);

  private = GET_PRIVATE (area);

  private->orig_width  = private->width  = orig_width;
  private->orig_height = private->height = orig_height;

  gimp_offset_area_resize (area);

  return GTK_WIDGET (area);
}

/**
 * gimp_offset_area_set_pixbuf:
 * @offset_area: a #GimpOffsetArea.
 * @pixbuf: a #GdkPixbuf.
 *
 * Sets the pixbuf which represents the original image/drawable which
 * is being offset.
 *
 * Since: 2.2
 **/
void
gimp_offset_area_set_pixbuf (GimpOffsetArea *area,
                             GdkPixbuf      *pixbuf)
{
  g_return_if_fail (GIMP_IS_OFFSET_AREA (area));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

  g_object_set_data_full (G_OBJECT (area), "pixbuf",
                          gdk_pixbuf_copy (pixbuf),
                          (GDestroyNotify) g_object_unref);

  gtk_widget_queue_draw (GTK_WIDGET (area));
}

/**
 * gimp_offset_area_set_size:
 * @offset_area: a #GimpOffsetArea.
 * @width: the new width
 * @height: the new height
 *
 * Sets the size of the image/drawable displayed by the #GimpOffsetArea.
 * If the offsets change as a result of this change, the "offsets-changed"
 * signal is emitted.
 **/
void
gimp_offset_area_set_size (GimpOffsetArea *area,
                           gint            width,
                           gint            height)
{
  GimpOffsetAreaPrivate *private;

  g_return_if_fail (GIMP_IS_OFFSET_AREA (area));
  g_return_if_fail (width > 0 && height > 0);

  private = GET_PRIVATE (area);

  if (private->width != width || private->height != height)
    {
      gint offset_x;
      gint offset_y;

      private->width  = width;
      private->height = height;

      if (private->orig_width <= private->width)
        offset_x = CLAMP (private->offset_x,
                          0, private->width - private->orig_width);
      else
        offset_x = CLAMP (private->offset_x,
                          private->width - private->orig_width, 0);

      if (private->orig_height <= private->height)
        offset_y = CLAMP (private->offset_y,
                          0, private->height - private->orig_height);
      else
        offset_y = CLAMP (private->offset_y,
                          private->height - private->orig_height, 0);

      if (offset_x != private->offset_x || offset_y != private->offset_y)
        {
          private->offset_x = offset_x;
          private->offset_y = offset_y;

          g_signal_emit (area,
                         gimp_offset_area_signals[OFFSETS_CHANGED], 0,
                         offset_x, offset_y);
        }

      gimp_offset_area_resize (area);
    }
}

/**
 * gimp_offset_area_set_offsets:
 * @offset_area: a #GimpOffsetArea.
 * @offset_x: the X offset
 * @offset_y: the Y offset
 *
 * Sets the offsets of the image/drawable displayed by the #GimpOffsetArea.
 * It does not emit the "offsets-changed" signal.
 **/
void
gimp_offset_area_set_offsets (GimpOffsetArea *area,
                              gint            offset_x,
                              gint            offset_y)
{
  GimpOffsetAreaPrivate *private;

  g_return_if_fail (GIMP_IS_OFFSET_AREA (area));

  private = GET_PRIVATE (area);

  if (private->offset_x != offset_x || private->offset_y != offset_y)
    {
      if (private->orig_width <= private->width)
        private->offset_x = CLAMP (offset_x,
                                   0, private->width - private->orig_width);
      else
        private->offset_x = CLAMP (offset_x,
                                   private->width - private->orig_width, 0);

      if (private->orig_height <= private->height)
        private->offset_y = CLAMP (offset_y,
                                   0, private->height - private->orig_height);
      else
        private->offset_y = CLAMP (offset_y,
                                   private->height - private->orig_height, 0);

      gtk_widget_queue_draw (GTK_WIDGET (area));
    }
}

static void
gimp_offset_area_resize (GimpOffsetArea *area)
{
  GimpOffsetAreaPrivate *private = GET_PRIVATE (area);
  gint                   width;
  gint                   height;
  gdouble                ratio;

  if (private->orig_width == 0 || private->orig_height == 0)
    return;

  if (private->orig_width <= private->width)
    width = private->width;
  else
    width = private->orig_width * 2 - private->width;

  if (private->orig_height <= private->height)
    height = private->height;
  else
    height = private->orig_height * 2 - private->height;

  ratio = (gdouble) DRAWING_AREA_SIZE / (gdouble) MAX (width, height);

  width  = ratio * (gdouble) width;
  height = ratio * (gdouble) height;

  gtk_widget_set_size_request (GTK_WIDGET (area), width, height);
  gtk_widget_queue_resize (GTK_WIDGET (area));
}

static void
gimp_offset_area_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  GimpOffsetArea        *area    = GIMP_OFFSET_AREA (widget);
  GimpOffsetAreaPrivate *private = GET_PRIVATE (area);
  GdkPixbuf             *pixbuf;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  private->display_ratio_x = ((gdouble) allocation->width /
                              ((private->orig_width <= private->width) ?
                               private->width :
                               private->orig_width * 2 - private->width));

  private->display_ratio_y = ((gdouble) allocation->height /
                              ((private->orig_height <= private->height) ?
                               private->height :
                               private->orig_height * 2 - private->height));

  pixbuf = g_object_get_data (G_OBJECT (area), "pixbuf");

  if (pixbuf)
    {
      GdkPixbuf *copy;
      gint       pixbuf_width;
      gint       pixbuf_height;

      pixbuf_width  = private->display_ratio_x * private->orig_width;
      pixbuf_width  = MAX (pixbuf_width, 1);

      pixbuf_height = private->display_ratio_y * private->orig_height;
      pixbuf_height = MAX (pixbuf_height, 1);

      copy = g_object_get_data (G_OBJECT (area), "pixbuf-copy");

      if (copy &&
          (pixbuf_width  != gdk_pixbuf_get_width (copy) ||
           pixbuf_height != gdk_pixbuf_get_height (copy)))
        {
          copy = NULL;
        }

      if (! copy)
        {
          copy = gdk_pixbuf_scale_simple (pixbuf, pixbuf_width, pixbuf_height,
                                          GDK_INTERP_NEAREST);

          g_object_set_data_full (G_OBJECT (area), "pixbuf-copy",
                                  copy, (GDestroyNotify) g_object_unref);
        }
    }
}

static void
gimp_offset_area_realize (GtkWidget *widget)
{
  GdkCursor *cursor;

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                       GDK_FLEUR);
  gdk_window_set_cursor (gtk_widget_get_window (widget), cursor);
  g_object_unref (cursor);
}

static gboolean
gimp_offset_area_event (GtkWidget *widget,
                        GdkEvent  *event)
{
  static gint orig_offset_x = 0;
  static gint orig_offset_y = 0;
  static gint start_x       = 0;
  static gint start_y       = 0;

  GimpOffsetArea        *area    = GIMP_OFFSET_AREA (widget);
  GimpOffsetAreaPrivate *private = GET_PRIVATE (area);
  gint                   offset_x;
  gint                   offset_y;

  if (private->orig_width == 0 || private->orig_height == 0)
    return FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 1)
        {
          gtk_grab_add (widget);

          orig_offset_x = private->offset_x;
          orig_offset_y = private->offset_y;
          start_x = event->button.x;
          start_y = event->button.y;
        }
      break;

    case GDK_MOTION_NOTIFY:
      offset_x = (orig_offset_x +
                  (event->motion.x - start_x) / private->display_ratio_x);
      offset_y = (orig_offset_y +
                  (event->motion.y - start_y) / private->display_ratio_y);

      if (private->offset_x != offset_x || private->offset_y != offset_y)
        {
          gimp_offset_area_set_offsets (area, offset_x, offset_y);

          g_signal_emit (area,
                         gimp_offset_area_signals[OFFSETS_CHANGED], 0,
                         private->offset_x, private->offset_y);
        }
      break;

    case GDK_BUTTON_RELEASE:
      if (event->button.button == 1)
        {
          gtk_grab_remove (widget);

          start_x = start_y = 0;
        }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static gboolean
gimp_offset_area_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  GimpOffsetArea        *area    = GIMP_OFFSET_AREA (widget);
  GimpOffsetAreaPrivate *private = GET_PRIVATE (area);
  GtkStyleContext       *context = gtk_widget_get_style_context (widget);
  GtkAllocation          allocation;
  GdkPixbuf             *pixbuf;
  gint                   w, h;
  gint                   x, y;

  gtk_widget_get_allocation (widget, &allocation);

  x = (private->display_ratio_x *
       ((private->orig_width <= private->width) ?
        private->offset_x :
        private->offset_x + private->orig_width - private->width));

  y = (private->display_ratio_y *
       ((private->orig_height <= private->height) ?
        private->offset_y :
        private->offset_y + private->orig_height - private->height));

  w = private->display_ratio_x * private->orig_width;
  w = MAX (w, 1);

  h = private->display_ratio_y * private->orig_height;
  h = MAX (h, 1);

  pixbuf = g_object_get_data (G_OBJECT (widget), "pixbuf-copy");

  if (pixbuf)
    {
      gdk_cairo_set_source_pixbuf (cr, pixbuf, x, y);
      cairo_paint (cr);

      cairo_rectangle (cr, x + 0.5, y + 0.5, w - 1, h - 1);
      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
      cairo_stroke (cr);
    }
  else
    {
      gtk_render_frame (context, cr, x, y, w, h);
    }

  if (private->orig_width > private->width ||
      private->orig_height > private->height)
    {
      gint line_width;

       if (private->orig_width > private->width)
        {
          x = private->display_ratio_x * (private->orig_width - private->width);
          w = private->display_ratio_x * private->width;
        }
      else
        {
          x = -1;
          w = allocation.width + 2;
        }

      if (private->orig_height > private->height)
        {
          y = private->display_ratio_y * (private->orig_height - private->height);
          h = private->display_ratio_y * private->height;
        }
      else
        {
          y = -1;
          h = allocation.height + 2;
        }

      w = MAX (w, 1);
      h = MAX (h, 1);

      line_width = MIN (3, MIN (w, h));

      cairo_rectangle (cr,
                       x + line_width / 2.0,
                       y + line_width / 2.0,
                       MAX (w - line_width, 1),
                       MAX (h - line_width, 1));

      cairo_set_line_width (cr, line_width);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
      cairo_stroke (cr);
    }

  return FALSE;
}
