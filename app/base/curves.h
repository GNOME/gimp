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
#ifndef __CURVES_H__
#define __CURVES_H__

#include <gtk/gtk.h>
#include "image_map.h"
#include "lut_funcs.h"
#include "tools.h"

#define SMOOTH 0
#define GFREE  1

typedef struct _CurvesDialog CurvesDialog;

struct _CurvesDialog
{
  GtkWidget    *shell;

  GtkWidget    *channel_menu;
  GtkWidget    *xrange;
  GtkWidget    *yrange;
  GtkWidget    *graph;
  GdkPixmap    *pixmap;

  GimpDrawable *drawable;
  ImageMap      image_map;

  gint          color;
  gint          channel;
  gboolean      preview;

  gint          grab_point;
  gint          last;
  gint          leftmost;
  gint          rightmost;
  gint          curve_type;
  gint          points[5][17][2];
  guchar        curve[5][256];
  gint          col_value[5];
  
  gint          cursor_ind_height;
  gint          cursor_ind_width;
  gint          cursor_ind_ascent;

  GimpLut      *lut;
};

Tool * tools_new_curves       (void);
void   tools_free_curves      (Tool         *tool);

void   curves_initialize      (GDisplay     *gdisp);
void   curves_free            (void);
float  curves_lut_func        (CurvesDialog *cd,
			       gint          nchannels,
			       gint          channel,
			       gfloat        value);
void   curves_calculate_curve (CurvesDialog *cd);

#endif  /*  __CURVES_H__  */
