/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpstrokeoptions.h
 * Copyright (C) 2003 Simon Budig
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

#ifndef __GIMP_STROKE_OPTIONS_H__
#define __GIMP_STROKE_OPTIONS_H__


#include "gimpcontext.h"


#define GIMP_TYPE_STROKE_OPTIONS            (gimp_stroke_options_get_type ())
#define GIMP_STROKE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STROKE_OPTIONS, GimpStrokeOptions))
#define GIMP_STROKE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STROKE_OPTIONS, GimpStrokeOptionsClass))
#define GIMP_IS_STROKE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STROKE_OPTIONS))
#define GIMP_IS_STROKE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_STROKE_OPTIONS))
#define GIMP_STROKE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_STROKE_OPTIONS, GimpStrokeOptionsClass))


typedef struct _GimpStrokeOptionsClass GimpStrokeOptionsClass;

struct _GimpStrokeOptions
{
  GimpContext      parent_instance;

  GimpStrokeStyle  style;

  gdouble          width;
  GimpUnit         unit;

  GimpCapStyle     cap_style;
  GimpJoinStyle    join_style;

  gdouble          miter_limit;

  gboolean         antialias;

  gdouble          dash_offset;
  GArray          *dash_info;
};

struct _GimpStrokeOptionsClass
{
  GimpContextClass parent_class;

  void (* dash_info_changed) (GimpStrokeOptions *stroke_options,
                              GimpDashPreset     preset);
};


GType  gimp_stroke_options_get_type         (void) G_GNUC_CONST;

void   gimp_stroke_options_set_dash_pattern (GimpStrokeOptions *options,
                                             GimpDashPreset     preset,
                                             GArray            *pattern);


#endif  /*  __GIMP_STROKE_OPTIONS_H__  */
