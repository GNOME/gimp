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

#ifndef __GIMP_CURVES_TOOL_H__
#define __GIMP_CURVES_TOOL_H__


#include "gimpimagemaptool.h"


#define SMOOTH 0
#define GFREE  1


#define GIMP_TYPE_CURVES_TOOL            (gimp_curves_tool_get_type ())
#define GIMP_CURVES_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CURVES_TOOL, GimpCurvesTool))
#define GIMP_IS_CURVES_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CURVES_TOOL))
#define GIMP_CURVES_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CURVES_TOOL, GimpCurvesToolClass))
#define GIMP_IS_CURVES_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CURVES_TOOL))


typedef struct _GimpCurvesTool      GimpCurvesTool;
typedef struct _GimpCurvesToolClass GimpCurvesToolClass;

struct _GimpCurvesTool
{
  GimpImageMapTool  parent_instance;
};

struct _GimpCurvesToolClass
{
  GimpImageMapToolClass  parent_class;
};


typedef struct _CurvesDialog CurvesDialog;

struct _CurvesDialog
{
  GtkWidget      *shell;

  GtkWidget      *channel_menu;
  GtkWidget      *xrange;
  GtkWidget      *yrange;
  GtkWidget      *graph;
  GdkPixmap      *pixmap;
  GtkWidget      *curve_type_menu;

  PangoLayout    *xpos_layout;
  PangoLayout    *cursor_layout;
  PangoRectangle  cursor_rect;

  GimpDrawable   *drawable;
  ImageMap       *image_map;

  gint            color;
  gint            channel;
  gboolean        preview;

  gint            grab_point;
  gint            last;
  gint            leftmost;
  gint            rightmost;
  gint            curve_type[5];
  gint            points[5][17][2];
  guchar          curve[5][256];
  gint            col_value[5];

  GimpLut        *lut;
};


void    gimp_curves_tool_register (Gimp *gimp);

GType   gimp_curves_tool_get_type (void);


void    curves_free            (void);
gfloat  curves_lut_func        (CurvesDialog *cd,
				gint          nchannels,
				gint          channel,
				gfloat        value);
void    curves_calculate_curve (CurvesDialog *cd);


#endif  /*  __CURVES_H__  */
