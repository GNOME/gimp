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


#include "gimptool.h"


/* FIXME: remove the dependency from pdb/color_cmds.c */
#include "widgets/widgets-types.h"


#define HISTOGRAM_WIDTH  256
#define HISTOGRAM_HEIGHT 150


#define GIMP_TYPE_HISTOGRAM_TOOL            (gimp_histogram_tool_get_type ())
#define GIMP_HISTOGRAM_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_HISTOGRAM_TOOL, GimpHistogramTool))
#define GIMP_IS_HISTOGRAM_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_HISTOGRAM_TOOL))
#define GIMP_HISTOGRAM_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HISTOGRAM_TOOL, GimpHistogramToolClass))
#define GIMP_IS_HISTOGRAM_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HISTOGRAM_TOOL))


typedef struct _GimpHistogramTool      GimpHistogramTool;
typedef struct _GimpHistogramToolClass GimpHistogramToolClass;

struct _GimpHistogramTool
{
  GimpTool  parent_instance;
};

struct _GimpHistogramToolClass
{
  GimpToolClass  parent_class;
};


typedef struct _HistogramToolDialog HistogramToolDialog;

struct _HistogramToolDialog
{
  GtkWidget       *shell;

  GtkWidget       *info_labels[7];
  GtkWidget       *channel_menu;
  HistogramWidget *histogram;
  GimpHistogram   *hist;
  GtkWidget       *gradient;

  gdouble          mean;
  gdouble          std_dev;
  gdouble          median;
  gdouble          pixels;
  gdouble          count;
  gdouble          percentile;

  GimpDrawable    *drawable;
  ImageMap        *image_map;
  gint             channel;
  gint             color;
};


void       gimp_histogram_tool_register (void);

GtkType    gimp_histogram_tool_get_type (void);


void   histogram_dialog_hide          (void);
void   histogram_tool_free            (void);
void   histogram_tool_histogram_range (HistogramWidget *hw,
				       gint             start,
				       gint             end,
				       gpointer         data);


#endif /* __HISTOGRAM_TOOL_H__ */
