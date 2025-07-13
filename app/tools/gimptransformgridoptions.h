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

#include "gimptransformoptions.h"


#define GIMP_TYPE_TRANSFORM_GRID_OPTIONS            (gimp_transform_grid_options_get_type ())
#define GIMP_TRANSFORM_GRID_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_GRID_OPTIONS, GimpTransformGridOptions))
#define GIMP_TRANSFORM_GRID_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_GRID_OPTIONS, GimpTransformGridOptionsClass))
#define GIMP_IS_TRANSFORM_GRID_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_GRID_OPTIONS))
#define GIMP_IS_TRANSFORM_GRID_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_GRID_OPTIONS))
#define GIMP_TRANSFORM_GRID_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_GRID_OPTIONS, GimpTransformGridOptionsClass))


typedef struct _GimpTransformGridOptions      GimpTransformGridOptions;
typedef struct _GimpTransformGridOptionsClass GimpTransformGridOptionsClass;

struct _GimpTransformGridOptions
{
  GimpTransformOptions  parent_instance;

  gboolean              direction_linked;
  gboolean              show_preview;
  gboolean              composited_preview;
  gboolean              synchronous_preview;
  gdouble               preview_opacity;
  GimpGuidesType        grid_type;
  gint                  grid_size;
  gboolean              constrain_move;
  gboolean              constrain_scale;
  gboolean              constrain_rotate;
  gboolean              constrain_shear;
  gboolean              constrain_perspective;
  gboolean              frompivot_scale;
  gboolean              frompivot_shear;
  gboolean              frompivot_perspective;
  gboolean              cornersnap;
  gboolean              fixedpivot;

  /*  options gui  */
  GtkWidget            *direction_chain_button;
};

struct _GimpTransformGridOptionsClass
{
  GimpTransformOptionsClass  parent_class;
};


GType       gimp_transform_grid_options_get_type     (void) G_GNUC_CONST;

GtkWidget * gimp_transform_grid_options_gui          (GimpToolOptions          *tool_options);

gboolean    gimp_transform_grid_options_show_preview (GimpTransformGridOptions *options);
