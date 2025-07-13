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


#define GIMP_TYPE_PATH_OPTIONS            (gimp_path_options_get_type ())
#define GIMP_PATH_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PATH_OPTIONS, GimpPathOptions))
#define GIMP_PATH_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PATH_OPTIONS, GimpPathOptionsClass))
#define GIMP_IS_PATH_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PATH_OPTIONS))
#define GIMP_IS_PATH_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PATH_OPTIONS))
#define GIMP_PATH_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PATH_OPTIONS, GimpPathOptionsClass))


typedef struct _GimpPathOptions      GimpPathOptions;
typedef struct _GimpToolOptionsClass GimpPathOptionsClass;

struct _GimpPathOptions
{
  GimpToolOptions    parent_instance;

  GimpPathMode       edit_mode;
  gboolean           polygonal;

  /* vector layer */
  gboolean           enable_fill;
  gboolean           enable_stroke;
  GimpFillOptions   *fill_options;
  GimpStrokeOptions *stroke_options;

  GimpCustomStyle    fill_style;
  GeglColor         *fill_foreground;
  GimpPattern       *fill_pattern;
  gboolean           fill_antialias;

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

  /*  options gui  */
  GtkWidget         *to_selection_button;
  GtkWidget         *vector_layer_button;
};


GType       gimp_path_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_path_options_gui      (GimpToolOptions *tool_options);
