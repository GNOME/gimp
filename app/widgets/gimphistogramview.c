/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "base/gimphistogram.h"

#include "core/gimpmarshal.h"

#include "gimphistogramview.h"


#define HISTOGRAM_MASK GDK_EXPOSURE_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
                       GDK_BUTTON_RELEASE_MASK | \
		       GDK_BUTTON1_MOTION_MASK

#define HISTOGRAM 0x1
#define RANGE     0x2
#define ALL       0xF


enum
{
  RANGE_CHANGED,
  LAST_SIGNAL
};


static void   gimp_histogram_view_class_init (GimpHistogramViewClass *klass);
static void   gimp_histogram_view_init       (GimpHistogramView      *view);


static guint histogram_view_signals[LAST_SIGNAL] = { 0 };

static GtkDrawingAreaClass *parent_class = NULL;


GType
gimp_histogram_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpHistogramViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_histogram_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpHistogramView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_histogram_view_init,
      };

      view_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                          "GimpHistogramView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_histogram_view_class_init (GimpHistogramViewClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  histogram_view_signals[RANGE_CHANGED] =
    g_signal_new ("range_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpHistogramViewClass, range_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__INT_INT,
		  G_TYPE_NONE, 2,
		  G_TYPE_INT,
		  G_TYPE_INT);

  klass->range_changed = NULL;
}

static void 
gimp_histogram_view_init (GimpHistogramView *view)
{
  view->histogram = NULL;
  view->channel   = GIMP_HISTOGRAM_VALUE;
  view->start     = 0;
  view->end       = 255;
}

static void
gimp_histogram_view_draw (GimpHistogramView *view,
                          gint               update)
{
  GtkWidget *widget;
  gdouble    max;
  gdouble    v;
  gint       i, x, y;
  gint       x1, x2;
  gint       width, height;

  widget = GTK_WIDGET (view);

  width  = widget->allocation.width - 2;
  height = widget->allocation.height - 2;

  if (update & HISTOGRAM)
    {
      /*  find the maximum value  */
      max = gimp_histogram_get_maximum (view->histogram,
					view->channel);

      if (max > 0.0)
	max = log (max);
      else
	max = 1.0;

      /*  clear the histogram  */
      gdk_window_clear (widget->window);

      /*  Draw the axis  */
      gdk_draw_line (widget->window,
		     widget->style->black_gc,
		     1, height + 1, width, height + 1);

      /*  Draw the spikes  */
      for (i = 0; i < 256; i++)
	{
	  x = (width * i) / 256 + 1;
	  v = gimp_histogram_get_value (view->histogram,
					view->channel, i);
	  if (v > 0.0)
	    y = (int) ((height * log (v)) / max);
	  else
	    y = 0;
	  gdk_draw_line (widget->window,
			 widget->style->black_gc,
			 x, height + 1,
			 x, height + 1 - y);
	}
    }

  if ((update & RANGE) && view->start >= 0)
    {
      x1 = (width * MIN (view->start, view->end)) / 256 + 1;
      x2 = (width * MAX (view->start, view->end)) / 256 + 1;

      gdk_gc_set_function (widget->style->black_gc, GDK_INVERT);
      gdk_draw_rectangle (widget->window,
			  widget->style->black_gc, TRUE,
			  x1, 1, (x2 - x1) + 1, height);
      gdk_gc_set_function (widget->style->black_gc, GDK_COPY);
    }
}

static gint
gimp_histogram_view_events (GimpHistogramView *view,
                            GdkEvent          *event)
{
  GtkWidget      *widget;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint            width;

  widget = GTK_WIDGET (view);

  switch (event->type)
    {
    case GDK_EXPOSE:
      gimp_histogram_view_draw (view, ALL);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button != 1)
	break;

      gdk_pointer_grab (widget->window, FALSE, 
			GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK,
			NULL, NULL, bevent->time);

      width = widget->allocation.width - 2;

      gimp_histogram_view_draw (view, RANGE);

      view->start = CLAMP ((((bevent->x - 1) * 256) / width), 0, 255);
      view->end = view->start;

      gimp_histogram_view_draw (view, RANGE);
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      gdk_pointer_ungrab (bevent->time);

      g_signal_emit (G_OBJECT (view),
                     histogram_view_signals[RANGE_CHANGED], 0,
                     MIN (view->start, view->end),
                     MAX (view->start, view->end));
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      width = widget->allocation.width - 2;

      gimp_histogram_view_draw (view, RANGE);

      view->start = CLAMP ((((mevent->x - 1) * 256) / width), 0, 255);

      gimp_histogram_view_draw (view, RANGE);
      break;

    default:
      break;
    }

  return FALSE;
}

GimpHistogramView *
gimp_histogram_view_new (gint width,
                         gint height)
{
  GimpHistogramView *view;

  view = g_object_new (GIMP_TYPE_HISTOGRAM_VIEW, NULL);

  gtk_widget_set_size_request (GTK_WIDGET (view), width + 2, height + 2);
  gtk_widget_set_events (GTK_WIDGET (view), HISTOGRAM_MASK);

  g_signal_connect (G_OBJECT (view), "event",
                    G_CALLBACK (gimp_histogram_view_events),
                    view);

  return view;
}

void
gimp_histogram_view_update (GimpHistogramView *view,
                            GimpHistogram     *histogram)
{
  view->histogram = histogram;

  if (! histogram)
    return;

  if (view->channel >= gimp_histogram_nchannels (histogram))
    gimp_histogram_view_channel (view, 0);

  gtk_widget_queue_draw (GTK_WIDGET (view));

  g_signal_emit (G_OBJECT (view),
                 histogram_view_signals[RANGE_CHANGED], 0,
                 MIN (view->start, view->end),
                 MAX (view->start, view->end));
}

void
gimp_histogram_view_range (GimpHistogramView *view,
                           gint               start,
                           gint               end)
{
  gimp_histogram_view_draw (view, RANGE);

  view->start = start;
  view->end   = end;

  gimp_histogram_view_draw (view, RANGE);
}

void
gimp_histogram_view_channel (GimpHistogramView *view,
                             gint               channel)
{
  view->channel = channel;

  gimp_histogram_view_draw (view, ALL);

  g_signal_emit (G_OBJECT (view),
                 histogram_view_signals[RANGE_CHANGED], 0,
                 MIN (view->start, view->end),
                 MAX (view->start, view->end));
}

GimpHistogram *
gimp_histogram_view_histogram (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), NULL);

  return view->histogram;
}
