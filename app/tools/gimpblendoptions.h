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

#ifndef  __GIMP_BLEND_OPTIONS_H__
#define  __GIMP_BLEND_OPTIONS_H__


#include "paint/gimppaintoptions.h"


#define GIMP_TYPE_BLEND_OPTIONS            (gimp_blend_options_get_type ())
#define GIMP_BLEND_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BLEND_OPTIONS, GimpBlendOptions))
#define GIMP_BLEND_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BLEND_OPTIONS, GimpBlendOptionsClass))
#define GIMP_IS_BLEND_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BLEND_OPTIONS))
#define GIMP_IS_BLEND_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BLEND_OPTIONS))
#define GIMP_BLEND_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BLEND_OPTIONS, GimpBlendOptionsClass))


typedef struct _GimpBlendOptions      GimpBlendOptions;
typedef struct _GimpPaintOptionsClass GimpBlendOptionsClass;

struct _GimpBlendOptions
{
  GimpPaintOptions  paint_options;

  gdouble           offset;
  gdouble           offset_d;
  GtkObject        *offset_w;

  GimpGradientType  gradient_type;
  GimpGradientType  gradient_type_d;
  GtkWidget        *gradient_type_w;

  GimpRepeatMode    repeat;
  GimpRepeatMode    repeat_d;
  GtkWidget        *repeat_w;
   
  gint              supersample;
  gint              supersample_d;
  GtkWidget        *supersample_w;

  gint              max_depth;
  gint              max_depth_d;
  GtkObject        *max_depth_w;

  gdouble           threshold;
  gdouble           threshold_d;
  GtkObject        *threshold_w;
};


GType   gimp_blend_options_get_type (void) G_GNUC_CONST;

void    gimp_blend_options_gui      (GimpToolOptions *tool_options);


#endif  /*  __GIMP_BLEND_OPTIONS_H__  */
