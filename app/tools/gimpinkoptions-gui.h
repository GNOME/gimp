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

#ifndef  __GIMP_INK_OPTIONS_H__
#define  __GIMP_INK_OPTIONS_H__


#include "paint/gimppaintoptions.h"
#include "gimpinktool-blob.h"  /* only used by ink */


#define GIMP_TYPE_INK_OPTIONS            (gimp_ink_options_get_type ())
#define GIMP_INK_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_INK_OPTIONS, GimpInkOptions))
#define GIMP_INK_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_INK_OPTIONS, GimpInkOptionsClass))
#define GIMP_IS_INK_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_INK_OPTIONS))
#define GIMP_IS_INK_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_INK_OPTIONS))
#define GIMP_INK_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_INK_OPTIONS, GimpInkOptionsClass))


typedef struct _GimpInkOptions        GimpInkOptions;
typedef struct _GimpPaintOptionsClass GimpInkOptionsClass;
typedef struct _BrushWidget           BrushWidget;


struct _BrushWidget
{
  GtkWidget      *widget;
  gboolean        state;

  /* EEK */
  GimpInkOptions *ink_options;
};

struct _GimpInkOptions
{
  GimpPaintOptions  paint_options;

  gdouble           size;
  gdouble           size_d;
  GtkObject        *size_w;

  gdouble           sensitivity;
  gdouble           sensitivity_d;
  GtkObject        *sensitivity_w;

  gdouble           vel_sensitivity;
  gdouble           vel_sensitivity_d;
  GtkObject        *vel_sensitivity_w;

  gdouble           tilt_sensitivity;
  gdouble           tilt_sensitivity_d;
  GtkObject        *tilt_sensitivity_w;

  gdouble           tilt_angle;
  gdouble           tilt_angle_d;
  GtkObject        *tilt_angle_w;

  BlobFunc          function;
  BlobFunc          function_d;
  GtkWidget        *function_w[3];  /* 3 radio buttons */

  gdouble           aspect;
  gdouble           aspect_d;
  gdouble           angle;
  gdouble           angle_d;
  BrushWidget      *brush_w;
};


GType   gimp_ink_options_get_type (void) G_GNUC_CONST;

void    gimp_ink_options_gui      (GimpToolOptions *tool_options);


#endif  /*  __GIMP_INK_OPTIONS_H__  */
