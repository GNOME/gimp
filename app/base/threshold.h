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

#ifndef __GIMP_THRESHOLD_TOOL_H__
#define __GIMP_THRESHOLD_TOOL_H__


#include "gimpimagemaptool.h"


#define GIMP_TYPE_THRESHOLD_TOOL            (gimp_threshold_tool_get_type ())
#define GIMP_THRESHOLD_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_THRESHOLD_TOOL, GimpThresholdTool))
#define GIMP_IS_THRESHOLD_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_THRESHOLD_TOOL))
#define GIMP_THRESHOLD_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_THRESHOLD_TOOL, GimpThresholdToolClass))
#define GIMP_IS_THRESHOLD_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_THRESHOLD_TOOL))


typedef struct _GimpThresholdTool      GimpThresholdTool;
typedef struct _GimpThresholdToolClass GimpThresholdToolClass;

struct _GimpThresholdTool
{
  GimpImageMapTool  parent_instance;
};

struct _GimpThresholdToolClass
{
  GimpImageMapToolClass  parent_class;
};


typedef struct _ThresholdDialog ThresholdDialog;

struct _ThresholdDialog
{
  GtkWidget       *shell;

  GtkAdjustment   *low_threshold_data;
  GtkAdjustment   *high_threshold_data;

  HistogramWidget *histogram;
  GimpHistogram   *hist;

  GimpDrawable    *drawable;
  ImageMap        *image_map;

  gint             color;
  gint             low_threshold;
  gint             high_threshold;

  gboolean         preview;
};


void       gimp_threshold_tool_register (Gimp *gimp);

GtkType    gimp_threshold_tool_get_type (void);


void   threshold_dialog_hide (void);
void   threshold_2           (gpointer     data,
			      PixelRegion *srcPR,
			      PixelRegion *destPR);


#endif  /*  __GIMP_THRESHOLD_TOOL_H__  */
