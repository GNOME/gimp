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
#ifndef __HISTOGRAM_TOOL_H__
#define __HISTOGRAM_TOOL_H__

#include "histogramwidget.h"
#include "image_map.h"
#include "tools.h"

#define HISTOGRAM_WIDTH 256
#define HISTOGRAM_HEIGHT 150

typedef struct _HistogramToolDialog HistogramToolDialog;
struct _HistogramToolDialog
{
  GtkWidget       *shell;
  GtkWidget       *info_labels[7];
  GtkWidget       *channel_menu;
  HistogramWidget *histogram;
  GimpHistogram   *hist;
  GtkWidget       *gradient;

  gdouble  mean;
  gdouble  std_dev;
  gdouble  median;
  gdouble  pixels;
  gdouble  count;
  gdouble  percentile;

  GimpDrawable *drawable;
  ImageMap      image_map;
  gint          channel;
  gint          color;
};

/*  histogram_tool functions  */
Tool * tools_new_histogram_tool  (void);
void   tools_free_histogram_tool (Tool *);

void   histogram_tool_initialize      (GDisplay *);
void   histogram_tool_free            (void);
void   histogram_tool_histogram_range (HistogramWidget *, gint, gint, void *);

#endif /* __HISTOGRAM_TOOL_H__ */
