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
#ifndef __THRESHOLD_H__
#define __THRESHOLD_H__

#include <gtk/gtk.h>
#include "gimpdrawableF.h"
#include "image_map.h"
#include "histogramwidget.h"
#include "tools.h"

typedef struct _ThresholdDialog ThresholdDialog;

struct _ThresholdDialog
{
  GtkWidget       *shell;

  GtkAdjustment   *low_threshold_data;
  GtkAdjustment   *high_threshold_data;

  HistogramWidget *histogram;
  GimpHistogram   *hist;

  GimpDrawable    *drawable;
  ImageMap         image_map;

  gint             color;
  gint             low_threshold;
  gint             high_threshold;

  gboolean         preview;
};

Tool * tools_new_threshold  (void);
void   tools_free_threshold (Tool        *tool);

void   threshold_initialize (GDisplay    *gdisp);
void   threshold_2          (void        *data,
			     PixelRegion *srcPR,
			     PixelRegion *destPR);

#endif  /*  __THRESHOLD_H__  */
