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
#include "histogram.h"

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

/*  Local structures  */
typedef struct _HistogramPrivate
{
  HistogramRangeCallback  range_callback;
  void *                  user_data;
  int                     channel;
  HistogramValues         values;
  int                     start;
  int                     end;
} HistogramPrivate;


/**************************/
/*  Function definitions  */

static void
histogram_draw (Histogram *histogram,
		int        update)
{
  HistogramPrivate *histogram_p;
  double max;
  double log_val;
  int i, x, y;
  int x1, x2;
  int width, height;

  histogram_p = (HistogramPrivate *) histogram->private_part;
  width = histogram->histogram_widget->allocation.width - 2;
  height = histogram->histogram_widget->allocation.height - 2;

  if (update & HISTOGRAM)
    {
      /*  find the maximum value  */
      max = 1.0;
      for (i = 0; i < 256; i++)
	{
	  if (histogram_p->values[histogram_p->channel][i])
	    log_val = log (histogram_p->values[histogram_p->channel][i]);
	  else
	    log_val = 0;

	  if (log_val > max)
	    max = log_val;
	}

      /*  clear the histogram  */
      gdk_window_clear (histogram->histogram_widget->window);

      /*  Draw the axis  */
      gdk_draw_line (histogram->histogram_widget->window,
		     histogram->histogram_widget->style->black_gc,
		     1, height + 1, width, height + 1);

      /*  Draw the spikes  */
      for (i = 0; i < 256; i++)
	{
	  x = (width * i) / 256 + 1;
	  if (histogram_p->values[histogram_p->channel][i])
	    y = (int) ((height * log (histogram_p->values[histogram_p->channel][i])) / max);
	  else
	    y = 0;
	  gdk_draw_line (histogram->histogram_widget->window,
			 histogram->histogram_widget->style->black_gc,
			 x, height + 1,
			 x, height + 1 - y);
	}
    }
  if ((update & RANGE) && histogram_p->start >= 0)
    {
      x1 = (width * MIN (histogram_p->start, histogram_p->end)) / 256 + 1;
      x2 = (width * MAX (histogram_p->start, histogram_p->end)) / 256 + 1;
      gdk_gc_set_function (histogram->histogram_widget->style->black_gc, GDK_INVERT);
      gdk_draw_rectangle (histogram->histogram_widget->window,
			  histogram->histogram_widget->style->black_gc, TRUE,
			  x1, 1, (x2 - x1) + 1, height);
      gdk_gc_set_function (histogram->histogram_widget->style->black_gc, GDK_COPY);
    }
}

static gint
histogram_events (GtkWidget *widget,
		  GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  Histogram *histogram;
  HistogramPrivate *histogram_p;
  int width;

  histogram = (Histogram *) gtk_object_get_user_data (GTK_OBJECT (widget));
  if (!histogram)
    return FALSE;
  histogram_p = (HistogramPrivate *) histogram->private_part;

  switch (event->type)
    {
    case GDK_EXPOSE:
      histogram_draw (histogram, ALL);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      width = histogram->histogram_widget->allocation.width - 2;

      histogram_draw (histogram, RANGE);

      histogram_p->start = BOUNDS ((((bevent->x - 1) * 256) / width), 0, 255);
      histogram_p->end = histogram_p->start;

      histogram_draw (histogram, RANGE);
      break;

    case GDK_BUTTON_RELEASE:
      (* histogram_p->range_callback) (MIN (histogram_p->start, histogram_p->end),
				       MAX (histogram_p->start, histogram_p->end),
				       histogram_p->values,
				       histogram_p->user_data);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      width = histogram->histogram_widget->allocation.width - 2;

      histogram_draw (histogram, RANGE);

      histogram_p->start = BOUNDS ((((mevent->x - 1) * 256) / width), 0, 255);

      histogram_draw (histogram, RANGE);
      break;

    default:
      break;
    }

  return FALSE;
}

Histogram *
histogram_create (int                     width,
		  int                     height,
		  HistogramRangeCallback  range_callback,
		  void                   *user_data)
{
  Histogram *histogram;
  HistogramPrivate *histogram_p;
  int i, j;

  histogram = (Histogram *) g_malloc (sizeof (Histogram));
  histogram->histogram_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (histogram->histogram_widget), width + 2, height + 2);
  gtk_widget_set_events (histogram->histogram_widget, HISTOGRAM_MASK);
  gtk_signal_connect (GTK_OBJECT (histogram->histogram_widget), "event",
		      (GtkSignalFunc) histogram_events, histogram);
  gtk_object_set_user_data (GTK_OBJECT (histogram->histogram_widget), (gpointer) histogram);

  /*  The private details of the histogram  */
  histogram_p = (HistogramPrivate *) g_malloc (sizeof (HistogramPrivate));
  histogram->private_part = (void *) histogram_p;
  histogram_p->range_callback = range_callback;
  histogram_p->user_data = user_data;
  histogram_p->channel = HISTOGRAM_VALUE;
  histogram_p->start = 0;
  histogram_p->end = 255;

  /*  Initialize the values array  */
  for (j = 0; j < 4; j++)
    for (i = 0; i < 256; i++)
      histogram_p->values[j][i] = 0.0;

  return histogram;
}

void
histogram_free (Histogram  *histogram)
{
  gtk_widget_destroy (histogram->histogram_widget);
  g_free (histogram->private_part);
  g_free (histogram);
}

void
histogram_update (Histogram         *histogram,
		  GimpDrawable      *drawable,
		  HistogramInfoFunc  info_func,
		  void              *user_data)
{
  HistogramPrivate *histogram_p;
  GImage *gimage;
  Channel *mask;
  PixelRegion srcPR, maskPR;
  int no_mask;
  void *pr;
  int x1, y1, x2, y2;
  int off_x, off_y;
  int i, j;

  histogram_p = (HistogramPrivate *) histogram->private_part;

  /*  Make sure the drawable is still valid  */
  if (! (gimage = drawable_gimage ( (drawable))))
    return;

  /*  The information collection should occur only within selection bounds  */
  no_mask = (drawable_mask_bounds ( (drawable), &x1, &y1, &x2, &y2) == FALSE);
  drawable_offsets ( (drawable), &off_x, &off_y);

  /*  Configure the src from the drawable data  */
  pixel_region_init (&srcPR, drawable_data ( (drawable)),
		     x1, y1, (x2 - x1), (y2 - y1), FALSE);

  /*  Configure the mask from the gimage's selection mask  */
  mask = gimage_get_mask (gimage);
  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)),
		     x1 + off_x, y1 + off_y, (x2 - x1), (y2 - y1), FALSE);

  /*  Initialize the values array  */
  for (j = 0; j < 4; j++)
    for (i = 0; i < 256; i++)
      histogram_p->values[j][i] = 0.0;

  /*  Apply the image transformation to the pixels  */
  if (no_mask)
    for (pr = pixel_regions_register (1, &srcPR); pr != NULL; pr = pixel_regions_process (pr))
      (* info_func) (&srcPR, NULL, histogram_p->values, user_data);
  else
    for (pr = pixel_regions_register (2, &srcPR, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
      (* info_func) (&srcPR, &maskPR, histogram_p->values, user_data);

  /*  Make sure the histogram is updated  */
  gtk_widget_draw (histogram->histogram_widget, NULL);

  /*  Give a range callback  */
  (* histogram_p->range_callback) (MIN (histogram_p->start, histogram_p->end),
				   MAX (histogram_p->start, histogram_p->end),
				   histogram_p->values,
				   histogram_p->user_data);
}

void
histogram_range (Histogram *histogram,
		 int        start,
		 int        end)
{
  HistogramPrivate *histogram_p;

  histogram_p = (HistogramPrivate *) histogram->private_part;

  histogram_draw (histogram, RANGE);
  histogram_p->start = start;
  histogram_p->end = end;
  histogram_draw (histogram, RANGE);
}

void
histogram_channel (Histogram *histogram,
		   int        channel)
{
  HistogramPrivate *histogram_p;

  histogram_p = (HistogramPrivate *) histogram->private_part;
  histogram_p->channel = channel;
  histogram_draw (histogram, ALL);

  /*  Give a range callback  */
  (* histogram_p->range_callback) (MIN (histogram_p->start, histogram_p->end),
				   MAX (histogram_p->start, histogram_p->end),
				   histogram_p->values,
				   histogram_p->user_data);
}

HistogramValues *
histogram_values (Histogram *histogram)
{
  HistogramPrivate *histogram_p;

  histogram_p = (HistogramPrivate *) histogram->private_part;

  return &histogram_p->values;
}
