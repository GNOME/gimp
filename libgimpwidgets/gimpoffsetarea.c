/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball 
 *
 * gimpoffsetarea.c
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpoffsetarea.h"


#define DRAWING_AREA_SIZE 200


enum
{
  OFFSETS_CHANGED,
  LAST_SIGNAL
};


static void      gimp_offset_area_class_init (GimpOffsetAreaClass *klass);
static void      gimp_offset_area_init       (GimpOffsetArea      *offset_area);

static void      gimp_offset_area_resize     (GimpOffsetArea      *offset_area);
static gboolean  gimp_offset_area_event      (GtkWidget           *widget,
                                              GdkEvent            *event);
static void      gimp_offset_area_draw       (GimpOffsetArea      *offset_area);


static guint gimp_offset_area_signals[LAST_SIGNAL] = { 0 };

static GtkDrawingAreaClass *parent_class = NULL;


GType
gimp_offset_area_get_type (void)
{
  static GType offset_area_type = 0;

  if (! offset_area_type)
    {
      static const GTypeInfo offset_area_info =
      {
        sizeof (GimpOffsetAreaClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_offset_area_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpOffsetArea),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_offset_area_init,
      };

      offset_area_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                                 "GimpOffsetArea",
                                                 &offset_area_info, 0);
    }

  return offset_area_type;
}

static void
gimp_offset_area_class_init (GimpOffsetAreaClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_offset_area_signals[OFFSETS_CHANGED] = 
    g_signal_new ("offsets_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpOffsetAreaClass, offsets_changed),
		  NULL, NULL,
		  gtk_marshal_VOID__INT_INT, 
		  G_TYPE_NONE, 2,
		  G_TYPE_INT,
		  G_TYPE_INT);

  widget_class->event   = gimp_offset_area_event;
}

static void
gimp_offset_area_init (GimpOffsetArea *offset_area)
{
  offset_area->orig_width      = 0;
  offset_area->orig_height     = 0;
  offset_area->width           = 0;
  offset_area->height          = 0;
  offset_area->offset_x        = 0;
  offset_area->offset_y        = 0;
  offset_area->display_ratio_x = 1.0;
  offset_area->display_ratio_y = 1.0;

  gtk_widget_add_events (GTK_WIDGET (offset_area), 
                         GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
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
  GimpOffsetArea *offset_area;

  g_return_val_if_fail (orig_width  > 0, NULL);
  g_return_val_if_fail (orig_height > 0, NULL);

  offset_area = g_object_new (GIMP_TYPE_OFFSET_AREA, NULL);

  offset_area->orig_width  = offset_area->width  = orig_width;
  offset_area->orig_height = offset_area->height = orig_height;

  gimp_offset_area_resize (offset_area);

  return GTK_WIDGET (offset_area);
}

/**
 * gimp_offset_area_set_size:
 * @offset_area: a #GimpOffsetArea.
 * @width: the new width
 * @height: the new height
 * 
 * Sets the size of the image/drawable displayed by the #GimpOffsetArea.
 * If the offsets change as a result of this change, the %offsets_changed
 * signal is emitted.
 **/
void
gimp_offset_area_set_size (GimpOffsetArea *offset_area,
                           gint            width,
                           gint            height)
{
  gint offset_x;
  gint offset_y;

  g_return_if_fail (GIMP_IS_OFFSET_AREA (offset_area));
  
  g_return_if_fail (width > 0 && height > 0);

  if (offset_area->width != width || offset_area->height != height)
    {
      offset_area->width  = width;     
      offset_area->height = height;

      if (offset_area->orig_width <= offset_area->width)
        offset_x = CLAMP (offset_area->offset_x, 
                          0, offset_area->width - offset_area->orig_width);
      else
        offset_x = CLAMP (offset_area->offset_x, 
                          offset_area->width - offset_area->orig_width, 0);

      if (offset_area->orig_height <= offset_area->height)
        offset_y = CLAMP (offset_area->offset_y, 
                          0, offset_area->height - offset_area->orig_height);
      else
        offset_y = CLAMP (offset_area->offset_y, 
                          offset_area->height - offset_area->orig_height, 0);

      offset_area->offset_x = offset_x;
      offset_area->offset_y = offset_y;

      gimp_offset_area_resize (offset_area);
      gtk_widget_queue_draw (GTK_WIDGET (offset_area));

      if (offset_x != offset_area->offset_x ||
          offset_y != offset_area->offset_y)
        {
          g_signal_emit (G_OBJECT (offset_area), 
                         gimp_offset_area_signals[OFFSETS_CHANGED], 0, 
                         offset_area->offset_x, 
                         offset_area->offset_y);
        }
    }
}

/**
 * gimp_offset_area_set_size:
 * @offset_area: a #GimpOffsetArea.
 * @width: the X offset
 * @height: the Y offset
 * 
 * Sets the offsets of the image/drawable displayed by the #GimpOffsetArea.
 * It does not emit the %offsets_changed signal.
 **/
void
gimp_offset_area_set_offsets (GimpOffsetArea *offset_area,
                              gint            offset_x,
                              gint            offset_y)
{
  g_return_if_fail (GIMP_IS_OFFSET_AREA (offset_area));
  
  if (offset_area->offset_x != offset_x || offset_area->offset_y != offset_y)
    {
      if (offset_area->orig_width <= offset_area->width)
        offset_area->offset_x = 
          CLAMP (offset_x, 0, offset_area->width - offset_area->orig_width);
      else
        offset_area->offset_x = 
          CLAMP (offset_x, offset_area->width - offset_area->orig_width, 0);

      if (offset_area->orig_height <= offset_area->height)
        offset_area->offset_y = 
          CLAMP (offset_y, 0, offset_area->height - offset_area->orig_height);
      else
        offset_area->offset_y = 
          CLAMP (offset_y, offset_area->height - offset_area->orig_height, 0);

      gtk_widget_queue_draw (GTK_WIDGET (offset_area));
    }
}

static void
gimp_offset_area_resize (GimpOffsetArea *offset_area)
{
  gint    width;
  gint    height;
  gdouble ratio;

  if (offset_area->orig_width == 0 || offset_area->orig_height == 0)
    return;

  if (offset_area->orig_width <= offset_area->width)
    width = offset_area->width;
  else
    width = offset_area->orig_width * 2 - offset_area->width;

  if (offset_area->orig_height <= offset_area->height)
    height = offset_area->height;
  else
    height = offset_area->orig_height * 2 - offset_area->height;

  ratio = (gdouble) DRAWING_AREA_SIZE / (gdouble) MAX (width, height);

  width  = ratio * (gdouble) width;
  height = ratio * (gdouble) height;

  gtk_drawing_area_size (GTK_DRAWING_AREA (offset_area), width, height);
}

static gboolean
gimp_offset_area_event (GtkWidget *widget,
                        GdkEvent  *event)
{
  static gint orig_offset_x = 0;
  static gint orig_offset_y = 0;
  static gint start_x       = 0;
  static gint start_y       = 0;
  
  GimpOffsetArea    *offset_area = GIMP_OFFSET_AREA (widget);
  GdkEventConfigure *conf_event;
  gint               offset_x;
  gint               offset_y;

  if (offset_area->orig_width == 0 || offset_area->orig_height == 0)
    return FALSE;

  switch (event->type)
    {
    case GDK_CONFIGURE:
      conf_event = (GdkEventConfigure *) event;
      offset_area->display_ratio_x = (gdouble) conf_event->width / 
        ((offset_area->orig_width <= offset_area->width) ?
         offset_area->width : 
         offset_area->orig_width * 2 - offset_area->width);

      offset_area->display_ratio_y = (gdouble) conf_event->height / 
        ((offset_area->orig_height <= offset_area->height) ?
         offset_area->height : 
         offset_area->orig_height * 2 - offset_area->height);
      break;
      
    case GDK_EXPOSE:
      gimp_offset_area_draw (offset_area);
      break;

    case GDK_BUTTON_PRESS:
      gdk_pointer_grab (widget->window, FALSE,
			(GDK_BUTTON1_MOTION_MASK |
			 GDK_BUTTON_RELEASE_MASK),
			NULL, NULL, event->button.time);

      orig_offset_x = offset_area->offset_x;
      orig_offset_y = offset_area->offset_y;
      start_x = event->button.x;
      start_y = event->button.y;
      break;

    case GDK_MOTION_NOTIFY:
      offset_x = orig_offset_x + 
        (event->motion.x - start_x) / offset_area->display_ratio_x;
      offset_y = orig_offset_y + 
        (event->motion.y - start_y) / offset_area->display_ratio_y;

      if (offset_area->offset_x != offset_x ||
          offset_area->offset_y != offset_y)
        {
          gimp_offset_area_set_offsets (offset_area, offset_x, offset_y);
          g_signal_emit (G_OBJECT (offset_area), 
                         gimp_offset_area_signals[OFFSETS_CHANGED], 0, 
                         offset_area->offset_x, 
                         offset_area->offset_y);
        }
      break;

    case GDK_BUTTON_RELEASE:
      gdk_pointer_ungrab (event->button.time);
      start_x = start_y = 0;
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_offset_area_draw (GimpOffsetArea *offset_area)
{
  GtkWidget *widget = GTK_WIDGET (offset_area);
  gint w, h;
  gint x, y;

  gdk_window_clear (widget->window);

  x = offset_area->display_ratio_x * 
    ((offset_area->orig_width <= offset_area->width) ? 
     offset_area->offset_x : 
     offset_area->offset_x + offset_area->orig_width - offset_area->width);

  y = offset_area->display_ratio_y * 
    ((offset_area->orig_height <= offset_area->height) ? 
     offset_area->offset_y : 
     offset_area->offset_y + offset_area->orig_height - offset_area->height);

  w = offset_area->display_ratio_x * offset_area->orig_width;
  h = offset_area->display_ratio_y * offset_area->orig_height;

  gtk_draw_shadow (widget->style, widget->window,
                   GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		   x, y, w, h);

  if (offset_area->orig_width > offset_area->width || 
      offset_area->orig_height > offset_area->height)
    {
      if (offset_area->orig_width > offset_area->width)
	{
	  x = offset_area->display_ratio_x * 
            (offset_area->orig_width - offset_area->width);
	  w = offset_area->display_ratio_x * offset_area->width;
	}
      else
	{
	  x = -1;
	  w = widget->allocation.width + 2;
	}
      if (offset_area->orig_height > offset_area->height)
	{
	  y = offset_area->display_ratio_y * 
            (offset_area->orig_height - offset_area->height);
	  h = offset_area->display_ratio_y * offset_area->height;
	}
      else
	{
	  y = -1;
	  h = widget->allocation.height + 2;
	}

      gdk_draw_rectangle (widget->window, widget->style->black_gc, 0,
			  x, y, w, h);
    }
}
