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
#ifndef __HISTOGRAM_WIDGET_H__
#define __HISTOGRAM_WIDGET_H__

#include <gtk/gtkdrawingarea.h>
#include "gimphistogram.h"

#define HISTOGRAM_WIDGET(obj) \
 GTK_CHECK_CAST (obj, histogram_widget_get_type (), HistogramWidget)
#define HISTOGRAM_WIDGET_CLASS(klass) \
 GTK_CHECK_CLASS_CAST (klass, histogram_widget_get_type (), HistogramWidget)
#define IS_HISTOGRAM_WIDGET(obj) \
 GTK_CHECK_TYPE (obj, histogram_widget_get_type ())

typedef struct _HistogramWidget HistogramWidget;
typedef struct _HistogramWidgetClass HistogramWidgetClass;

typedef struct _hist Histogram;

/* HistogramWidget signals:
     rangechanged
*/

struct _HistogramWidget
{
  GtkDrawingArea drawingarea;

  int                     channel;
  GimpHistogram          *histogram;
  int                     start;
  int                     end;
};

struct _HistogramWidgetClass
{
  GtkDrawingAreaClass parent_class;
};


/*  Histogram functions  */

guint histogram_widget_get_type ();

HistogramWidget *histogram_widget_new       (int width, int height);
void             histogram_widget_update    (HistogramWidget *,
					     GimpHistogram *);
void             histogram_widget_range     (HistogramWidget *, int, int);
void             histogram_widget_channel   (HistogramWidget *, int);
GimpHistogram   *histogram_widget_histogram (HistogramWidget *);

#endif /* __HISTOGRAM_WIDGET_H__ */




