/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpstrokeoptions.h
 * Copyright (C) 2003 Simon Budig
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "gimpfilloptions.h"


#define GIMP_TYPE_STROKE_OPTIONS (gimp_stroke_options_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpStrokeOptions,
                          gimp_stroke_options,
                          GIMP, STROKE_OPTIONS,
                          GimpFillOptions)


struct _GimpStrokeOptionsClass
{
  GimpFillOptionsClass  parent_class;

  void (* dash_info_changed) (GimpStrokeOptions *stroke_options,
                              GimpDashPreset     preset);
};


GimpStrokeOptions * gimp_stroke_options_new                  (Gimp              *gimp,
                                                              GimpContext       *context,
                                                              gboolean           use_context_color);

GimpStrokeMethod    gimp_stroke_options_get_method           (GimpStrokeOptions *options);

gdouble             gimp_stroke_options_get_width            (GimpStrokeOptions *options);
GimpUnit          * gimp_stroke_options_get_unit             (GimpStrokeOptions *options);
GimpCapStyle        gimp_stroke_options_get_cap_style        (GimpStrokeOptions *options);
GimpJoinStyle       gimp_stroke_options_get_join_style       (GimpStrokeOptions *options);
gdouble             gimp_stroke_options_get_miter_limit      (GimpStrokeOptions *options);
gdouble             gimp_stroke_options_get_dash_offset      (GimpStrokeOptions *options);
GArray            * gimp_stroke_options_get_dash_info        (GimpStrokeOptions *options);

GimpPaintOptions  * gimp_stroke_options_get_paint_options    (GimpStrokeOptions *options);
gboolean            gimp_stroke_options_get_emulate_dynamics (GimpStrokeOptions *options);

void                gimp_stroke_options_take_dash_pattern    (GimpStrokeOptions *options,
                                                              GimpDashPreset     preset,
                                                              GArray            *pattern);

void                gimp_stroke_options_prepare              (GimpStrokeOptions *options,
                                                              GimpContext       *context,
                                                              GimpPaintOptions  *paint_options);
void                gimp_stroke_options_finish               (GimpStrokeOptions *options);
