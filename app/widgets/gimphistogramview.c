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


#define MIN_HEIGHT 80

enum
{
  RANGE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_SCALE
};

static void  gimp_histogram_view_class_init   (GimpHistogramViewClass *klass);
static void  gimp_histogram_view_init         (GimpHistogramView *view);
static void  gimp_histogram_view_set_property (GObject           *object,
					       guint              property_id,
					       const GValue      *value,
					       GParamSpec        *pspec);
static void  gimp_histogram_view_get_property (GObject           *object,
					       guint              property_id,
					       GValue            *value,
					       GParamSpec        *pspec);
static void  gimp_histogram_view_unrealize    (GtkWidget         *widget);
static void  gimp_histogram_view_size_request (GtkWidget         *widget,
                                               GtkRequisition    *requisition);
static gboolean  gimp_histogram_view_expose   (GtkWidget         *widget,
					       GdkEventExpose    *event);


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
  GObjectClass   *object_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

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

  object_class->get_property = gimp_histogram_view_get_property;
  object_class->set_property = gimp_histogram_view_set_property;

  widget_class->unrealize    = gimp_histogram_view_unrealize;
  widget_class->size_request = gimp_histogram_view_size_request;
  widget_class->expose_event = gimp_histogram_view_expose;

  klass->range_changed = NULL;

  g_object_class_install_property (object_class, PROP_CHANNEL,
				   g_param_spec_enum ("histogram-channel", NULL, NULL,
						      GIMP_TYPE_HISTOGRAM_CHANNEL,
						      GIMP_HISTOGRAM_VALUE,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SCALE,
				   g_param_spec_enum ("histogram-scale", NULL, NULL,
						      GIMP_TYPE_HISTOGRAM_SCALE,
						      GIMP_HISTOGRAM_SCALE_LINEAR,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));
}

static void
gimp_histogram_view_init (GimpHistogramView *view)
{
  view->histogram = NULL;
  view->start     = 0;
  view->end       = 255;
}

static void
gimp_histogram_view_set_property (GObject      *object,
				  guint         property_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      view->channel = g_value_get_enum (value);
      break;
    case PROP_SCALE:
      view->scale = g_value_get_enum (value);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
gimp_histogram_view_get_property (GObject      *object,
				  guint         property_id,
				  GValue       *value,
				  GParamSpec   *pspec)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, view->channel);
      break;
    case PROP_SCALE:
      g_value_set_enum (value, view->scale);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_view_unrealize (GtkWidget *widget)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  if (view->range_gc)
    {
      g_object_unref (view->range_gc);
      view->range_gc = NULL;
    }

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_histogram_view_size_request (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
  requisition->width  = widget->requisition.width  + 2;
  requisition->height = MAX (MIN_HEIGHT, widget->requisition.height) + 2;
}

static gboolean
gimp_histogram_view_expose (GtkWidget      *widget,
                            GdkEventExpose *event)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);
  gint               x, y;
  gint               x1, x2;
  gint               width, height;
  gdouble            max;

  if (!view->histogram)
    return TRUE;

  width  = widget->allocation.width  - 2;
  height = widget->allocation.height - 2;

  /*  find the maximum value  */
  max = gimp_histogram_get_maximum (view->histogram, view->channel);

  switch (view->scale)
    {
    case GIMP_HISTOGRAM_SCALE_LINEAR:
      break;

    case GIMP_HISTOGRAM_SCALE_LOGARITHMIC:
      if (max > 0.0)
	max = log (max);
      else
	max = 1.0;
      break;
    }

  /*  Draw the axis  */
  gdk_draw_line (widget->window,
                 widget->style->black_gc,
                 1, height + 1, width, height + 1);

  /*  Draw the spikes  */
  for (x = 0; x < width; x++)
    {
      gdouble v, value = 0.0;
      gint    i, j;

      i = (x * 256) / width;
      j = ((x + 1) * 256) / width;

      do
        {
          v = gimp_histogram_get_value (view->histogram, view->channel, i++);

          if (v > value)
            value = v;
        }
      while (i < j);

      if (value <= 0.0)
	continue;

      switch (view->scale)
	{
	case GIMP_HISTOGRAM_SCALE_LINEAR:
	  y = (gint) ((height * value) / max);
	  break;

	case GIMP_HISTOGRAM_SCALE_LOGARITHMIC:
	  y = (gint) ((height * log (value)) / max);
	  break;

	default:
	  y = 0;
	  break;
	}

      gdk_draw_line (widget->window,
                     widget->style->black_gc,
                     x + 1, height + 1,
                     x + 1, height + 1 - y);
    }

  x1 = CLAMP (MIN (view->start, view->end), 0, 255);
  x2 = CLAMP (MAX (view->start, view->end), 0, 255);

  if (! (x1 == 0 && x2 == 255))
    {
      x1 = (x1 * width) / 256;
      x2 = (x2 * width) / 255;

      if (x2 == x1)
        x2++;

      if (!view->range_gc)
        {
          view->range_gc = gdk_gc_new (widget->window);
          gdk_gc_set_function (view->range_gc, GDK_INVERT);
        }

      gdk_draw_rectangle (widget->window, view->range_gc, TRUE,
                          x1 + 1, 1, (x2 - x1), height);
    }

  return TRUE;
}

static gboolean
gimp_histogram_view_events (GimpHistogramView *view,
                            GdkEvent          *event)
{
  GtkWidget      *widget = GTK_WIDGET (view);
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint            width;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button != 1)
	break;

      gdk_pointer_grab (widget->window, FALSE,
			GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK,
			NULL, NULL, bevent->time);

      width = widget->allocation.width - 2;

      view->start = CLAMP ((((bevent->x - 1) * 256) / width), 0, 255);
      view->end   = view->start;

      gtk_widget_queue_draw (widget);
      return TRUE;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      gdk_display_pointer_ungrab (gtk_widget_get_display (GTK_WIDGET (view)),
                                  bevent->time);

      {
        gint start, end;

        start = view->start;
        end   = view->end;

        view->start = MIN (start, end);
        view->end   = MAX (start, end);
      }

      g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                     view->start, view->end);
      return TRUE;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      width = widget->allocation.width - 2;

      view->start = CLAMP ((((mevent->x - 1) * 256) / width), 0, 255);

      gtk_widget_queue_draw (widget);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

GtkWidget *
gimp_histogram_view_new (gboolean range)
{
  GtkWidget *view = g_object_new (GIMP_TYPE_HISTOGRAM_VIEW, NULL);

  if (range)
    {
      gtk_widget_add_events (view,
                             GDK_BUTTON_PRESS_MASK   |
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_BUTTON1_MOTION_MASK);
    }
  else
    {
      GIMP_HISTOGRAM_VIEW (view)->start = -1;
      GIMP_HISTOGRAM_VIEW (view)->end   = -1;
    }

  g_signal_connect (view, "event",
                    G_CALLBACK (gimp_histogram_view_events),
                    view);

  return view;
}

void
gimp_histogram_view_set_histogram (GimpHistogramView *view,
                                   GimpHistogram     *histogram)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (view->histogram != histogram)
    {
      view->histogram = histogram;

      if (histogram && view->channel >= gimp_histogram_nchannels (histogram))
        gimp_histogram_view_set_channel (view, 0);
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_histogram_view_set_range (GimpHistogramView *view,
                               gint               start,
                               gint               end)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  view->start = MIN (start, end);
  view->end   = MAX (start, end);

  gtk_widget_queue_draw (GTK_WIDGET (view));

  g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                 view->start, view->end);
}

void
gimp_histogram_view_set_channel (GimpHistogramView    *view,
                                 GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  g_object_set (view, "histogram-channel", channel, NULL);
}

GimpHistogramChannel
gimp_histogram_view_get_channel (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), 0);

  return view->channel;
}

void
gimp_histogram_view_set_scale (GimpHistogramView  *view,
			       GimpHistogramScale  scale)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  g_object_set (view, "histogram-scale", scale, NULL);
}

GimpHistogram *
gimp_histogram_view_get_histogram (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), NULL);

  return view->histogram;
}
