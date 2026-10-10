/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "core/gimptooloptions.h"


typedef enum
{
  GIMP_SHAPE_TYPE_LINE,
  GIMP_SHAPE_TYPE_RECTANGLE,
  GIMP_SHAPE_TYPE_ARC,
  GIMP_SHAPE_TYPE_POLYGON,
  GIMP_SHAPE_TYPE_STAR,
  GIMP_SHAPE_TYPE_SPIRAL,

  GIMP_SHAPE_TYPE_LAST
} GimpShapeType;


#define GIMP_TYPE_SHAPE_OPTIONS            (gimp_shape_options_get_type ())
#define GIMP_SHAPE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SHAPE_OPTIONS, GimpShapeOptions))
#define GIMP_SHAPE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SHAPE_OPTIONS, GimpShapeOptionsClass))
#define GIMP_IS_SHAPE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SHAPE_OPTIONS))
#define GIMP_IS_SHAPE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SHAPE_OPTIONS))
#define GIMP_SHAPE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SHAPE_OPTIONS, GimpShapeOptionsClass))


typedef struct _GimpShapeOptions      GimpShapeOptions;
typedef struct _GimpToolOptionsClass  GimpShapeOptionsClass;

struct _GimpShapeOptions
{
  GimpToolOptions    parent_instance;

  GimpShapeType      shape_type;
  gboolean           draw_on_layers;
  gboolean           fixed_aspect_ratio;

  gint               number_of_sides;
  gint               spiral_direction;

  gboolean           enable_fill;
  GimpFillOptions   *fill_options;
  GimpCustomStyle    fill_style;
  GeglColor         *fill_foreground;
  GimpPattern       *fill_pattern;
  gboolean           fill_antialias;

  gboolean           enable_stroke;
  GimpStrokeOptions *stroke_options;
  GimpCustomStyle    stroke_style;
  GeglColor         *stroke_foreground;
  GimpPattern       *stroke_pattern;
  gboolean           stroke_antialias;
  gdouble            stroke_width;
  GimpUnit          *stroke_unit;
  GimpCapStyle       stroke_cap_style;
  GimpJoinStyle      stroke_join_style;
  gdouble            stroke_miter_limit;
  gdouble            stroke_dash_offset;

  GtkWidget         *fixed_aspect_button;
  GtkWidget         *n_sides_button;
  GtkWidget         *n_sides_label;
  GtkWidget         *spiral_direction_combo;
  GtkWidget         *spiral_direction_label;
};


GType       gimp_shape_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_shape_options_gui      (GimpToolOptions *tool_options);
