/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_PAINT_OPTIONS_H__
#define __GIMP_PAINT_OPTIONS_H__


#include "tools/tools-types.h"  /* temp hack */
#include "tools/tool_options.h" /* temp hack */


typedef struct _GimpPressureOptions GimpPressureOptions;

struct _GimpPressureOptions
{
  GtkWidget   *frame;

  gboolean     opacity;
  gboolean     opacity_d;
  GtkWidget   *opacity_w;

  gboolean     pressure;
  gboolean     pressure_d;
  GtkWidget   *pressure_w;

  gboolean     rate;
  gboolean     rate_d;
  GtkWidget   *rate_w;

  gboolean     size;
  gboolean     size_d;
  GtkWidget   *size_w;
  
  gboolean     color;
  gboolean     color_d;
  GtkWidget   *color_w;
};


typedef struct _GimpGradientOptions GimpGradientOptions;

struct _GimpGradientOptions
{
  GtkWidget    *frame;

  gboolean      use_fade;
  gboolean      use_fade_d;
  GtkWidget    *use_fade_w;

  gdouble       fade_out;
  gdouble       fade_out_d;
  GtkObject    *fade_out_w;

  GimpUnit      fade_unit;
  GimpUnit      fade_unit_d;
  GtkWidget    *fade_unit_w;

  gboolean      use_gradient;
  gboolean      use_gradient_d;
  GtkWidget    *use_gradient_w;

  gdouble       gradient_length;
  gdouble       gradient_length_d;
  GtkObject    *gradient_length_w;

  GimpUnit      gradient_unit;
  GimpUnit      gradient_unit_d;
  GtkWidget    *gradient_unit_w;

  gint          gradient_type;
  gint          gradient_type_d;
  GtkWidget    *gradient_type_w;
};


struct _GimpPaintOptions
{
  GimpToolOptions  tool_options;

  /*  vbox for the common paint options  */
  GtkWidget   *paint_vbox;

  /*  a widget to be shown if we are in global mode  */
  GtkWidget   *global;

  /*  options used by all paint tools  */
  GtkObject   *opacity_w;
  GtkWidget   *paint_mode_w;

  /*  this tool's private context  */
  GimpContext *context;

  /*  the incremental toggle  */
  gboolean     incremental;
  gboolean     incremental_d;
  GtkWidget   *incremental_w;

  gboolean     incremental_save;

  /*  the pressure-sensitivity options  */
  GimpPressureOptions *pressure_options;

  /*  the fade out and gradient options  */
  GimpGradientOptions *gradient_options;
};


/*  paint tool options functions  */

GimpPaintOptions * gimp_paint_options_new  (void);

void               gimp_paint_options_init (GimpPaintOptions *options);


#endif  /*  __GIMP_PAINT_OPTIONS_H__  */
