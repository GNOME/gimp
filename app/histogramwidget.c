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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "histogramwidget.h"
#include "tile_manager.h"

#define WAITING 0
#define WORKING 1

#define WORK_DELAY 1

#define HISTOGRAM_MASK GDK_EXPOSURE_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
                       GDK_BUTTON_RELEASE_MASK | \
		       GDK_BUTTON1_MOTION_MASK

#define HISTOGRAM 0x1
#define RANGE     0x2
#define ALL       0xF


enum{
  RANGED,
  LAST_SIGNAL
};

static guint histogram_widget_signals[LAST_SIGNAL];

static void   histogram_widget_class_init (HistogramWidgetClass *klass);
static void   histogram_widget_init       (HistogramWidget *histogram);


/**************************/
/*  Function definitions  */

guint
histogram_widget_get_type (void)
{
  static guint type = 0;
  if (!type)
  {
    static const GtkTypeInfo info =
    {
      "HistogramWidget",
      sizeof (HistogramWidget),
      sizeof (HistogramWidgetClass),
      (GtkClassInitFunc) histogram_widget_class_init,
      (GtkObjectInitFunc) histogram_widget_init,
      NULL, NULL
    };
    type = gtk_type_unique (gtk_drawing_area_get_type (), &info);
  }
  return type;
}

static void
histogram_widget_class_init (HistogramWidgetClass *klass)
{
  GtkObjectClass *object_class;
  GtkType type;

  object_class = GTK_OBJECT_CLASS(klass);
  type=object_class->type;

  histogram_widget_signals[RANGED] = gtk_signal_new("rangechanged",
						    GTK_RUN_FIRST, /* ? */
						    type,
						    0,
						    gtk_marshal_NONE__INT_INT,
						    GTK_TYPE_NONE,
						    2,
						    GTK_TYPE_INT,
						    GTK_TYPE_INT);

  gtk_object_class_add_signals (GTK_OBJECT_CLASS(klass),
				histogram_widget_signals, LAST_SIGNAL);

  
}

static void 
histogram_widget_init (HistogramWidget *histogram)
{
  histogram->histogram = NULL;
}

static void
histogram_widget_draw (HistogramWidget *histogram,
		       int              update)
{
  double max;
  double v;
  int i, x, y;
  int x1, x2;
  int width, height;

  width = GTK_WIDGET(histogram)->allocation.width - 2;
  height = GTK_WIDGET(histogram)->allocation.height - 2;

  if (update & HISTOGRAM)
    {
      /*  find the maximum value  */
      max = gimp_histogram_get_maximum(histogram->histogram,
				       histogram->channel);
      if (max > 0.0)
	max = log(max);
      else
	max = 1.0;

      /*  clear the histogram  */
      gdk_window_clear (GTK_WIDGET(histogram)->window);

      /*  Draw the axis  */
      gdk_draw_line (GTK_WIDGET(histogram)->window,
		     GTK_WIDGET(histogram)->style->black_gc,
		     1, height + 1, width, height + 1);

      /*  Draw the spikes  */
      for (i = 0; i < 256; i++)
	{
	  x = (width * i) / 256 + 1;
	  v = gimp_histogram_get_value(histogram->histogram,
				       histogram->channel, i);
	  if (v > 0.0)
	    y = (int) ((height * log (v)) / max);
	  else
	    y = 0;
	  gdk_draw_line (GTK_WIDGET(histogram)->window,
			 GTK_WIDGET(histogram)->style->black_gc,
			 x, height + 1,
			 x, height + 1 - y);
	}
    }
  if ((update & RANGE) && histogram->start >= 0)
    {
      x1 = (width * MIN (histogram->start, histogram->end)) / 256 + 1;
      x2 = (width * MAX (histogram->start, histogram->end)) / 256 + 1;
      gdk_gc_set_function (GTK_WIDGET(histogram)->style->black_gc, GDK_INVERT);
      gdk_draw_rectangle (GTK_WIDGET(histogram)->window,
			  GTK_WIDGET(histogram)->style->black_gc, TRUE,
			  x1, 1, (x2 - x1) + 1, height);
      gdk_gc_set_function (GTK_WIDGET(histogram)->style->black_gc, GDK_COPY);
    }
}

static gint
histogram_widget_events (HistogramWidget *histogram,
			 GdkEvent        *event)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int width;

  switch (event->type)
    {
    case GDK_EXPOSE:
      histogram_widget_draw (histogram, ALL);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button != 1)
	break;
      
      gdk_pointer_grab (GTK_WIDGET (histogram)->window, FALSE, 
			GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK,
			NULL, NULL, bevent->time);

      width = GTK_WIDGET(histogram)->allocation.width - 2;

      histogram_widget_draw (histogram, RANGE);

      histogram->start = CLAMP ((((bevent->x - 1) * 256) / width), 0, 255);
      histogram->end = histogram->start;

      histogram_widget_draw (histogram, RANGE);
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      gdk_pointer_ungrab (bevent->time);

      gtk_signal_emit(GTK_OBJECT(histogram), histogram_widget_signals[RANGED],
		      MIN (histogram->start, histogram->end),
		      MAX (histogram->start, histogram->end));
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      width = GTK_WIDGET(histogram)->allocation.width - 2;

      histogram_widget_draw (histogram, RANGE);

      histogram->start = CLAMP ((((mevent->x - 1) * 256) / width), 0, 255);

      histogram_widget_draw (histogram, RANGE);
      break;

    default:
      break;
    }

  return FALSE;
}

HistogramWidget *
histogram_widget_new (int width,
		      int height)
{
  HistogramWidget *histogram 
    = HISTOGRAM_WIDGET(gtk_type_new(histogram_widget_get_type()));

  gtk_drawing_area_size (GTK_DRAWING_AREA (histogram), width + 2, height + 2);
  gtk_widget_set_events (GTK_WIDGET(histogram), HISTOGRAM_MASK);
  gtk_signal_connect (GTK_OBJECT (histogram), "event",
		      (GtkSignalFunc) histogram_widget_events, histogram);

  /*  The private details of the histogram  */
  histogram->channel = HISTOGRAM_VALUE;
  histogram->start = 0;
  histogram->end = 255;
  histogram->histogram = NULL;

  return histogram;
}

void
histogram_widget_destroy (HistogramWidget *histogram)
{
  gtk_widget_destroy (GTK_WIDGET(histogram));
  g_free (histogram);
}

void
histogram_widget_update (HistogramWidget *histogram_widget,
			 GimpHistogram   *hist)
{
  histogram_widget->histogram = hist;
  if (!hist)
    return;
  if (histogram_widget->channel >= gimp_histogram_nchannels(hist))
    histogram_widget_channel(histogram_widget, 0);
  /*  Make sure the histogram is updated  */
  gtk_widget_draw (GTK_WIDGET(histogram_widget), NULL);

  /*  Give a range callback  */
  gtk_signal_emit(GTK_OBJECT(histogram_widget),
		  histogram_widget_signals[RANGED],
		  MIN (histogram_widget->start, histogram_widget->end),
		  MAX (histogram_widget->start, histogram_widget->end));
}

void
histogram_widget_range (HistogramWidget *histogram,
			int              start,
			int              end)
{
  histogram_widget_draw (histogram, RANGE);
  histogram->start = start;
  histogram->end = end;
  histogram_widget_draw (histogram, RANGE);
}

void
histogram_widget_channel (HistogramWidget *histogram,
			  int              channel)
{
  histogram->channel = channel;
  histogram_widget_draw (histogram, ALL);

  /*  Give a range callback  */
  gtk_signal_emit(GTK_OBJECT(histogram), histogram_widget_signals[RANGED],
		  MIN (histogram->start, histogram->end),
		  MAX (histogram->start, histogram->end));
}

GimpHistogram *
histogram_widget_histogram (HistogramWidget *histogram)
{
  return histogram->histogram;
}
