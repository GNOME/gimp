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

#include "gimppaintcore.h"


#define BRUSH_CORE_SUBSAMPLE        4
#define BRUSH_CORE_SOLID_SUBSAMPLE  2
#define BRUSH_CORE_JITTER_LUTSIZE   360


#define GIMP_TYPE_BRUSH_CORE            (gimp_brush_core_get_type ())
#define GIMP_BRUSH_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_CORE, GimpBrushCore))
#define GIMP_BRUSH_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_CORE, GimpBrushCoreClass))
#define GIMP_IS_BRUSH_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_CORE))
#define GIMP_IS_BRUSH_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_CORE))
#define GIMP_BRUSH_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_CORE, GimpBrushCoreClass))


typedef struct _GimpBrushCoreClass GimpBrushCoreClass;

struct _GimpBrushCore
{
  GimpPaintCore      parent_instance;

  GimpBrush         *main_brush;
  GimpBrush         *brush;
  GimpDynamics      *dynamics;
  gdouble            spacing;
  gdouble            scale;
  gdouble            aspect_ratio;
  gdouble            angle;
  gboolean           reflect;
  gdouble            hardness;

  gdouble            symmetry_angle;
  gboolean           symmetry_reflect;

  /*  brush buffers  */
  GimpTempBuf       *pressure_brush;

  GimpTempBuf       *solid_brushes[BRUSH_CORE_SOLID_SUBSAMPLE][BRUSH_CORE_SOLID_SUBSAMPLE];
  const GimpTempBuf *last_solid_brush_mask;
  gboolean           solid_cache_invalid;

  const GimpTempBuf *transform_brush;
  const GimpTempBuf *transform_pixmap;

  GimpTempBuf       *subsample_brushes[BRUSH_CORE_SUBSAMPLE + 1][BRUSH_CORE_SUBSAMPLE + 1];
  const GimpTempBuf *last_subsample_brush_mask;
  gboolean           subsample_cache_invalid;

  gdouble            jitter;
  gdouble            jitter_lut_x[BRUSH_CORE_JITTER_LUTSIZE];
  gdouble            jitter_lut_y[BRUSH_CORE_JITTER_LUTSIZE];

  GRand             *rand;
};

struct _GimpBrushCoreClass
{
  GimpPaintCoreClass  parent_class;

  /*  Set for tools that don't mind if the brush changes while painting  */
  gboolean            handles_changing_brush;

  /*  Set for tools that don't mind if the brush scales while painting  */
  gboolean            handles_transforming_brush;

  /*  Set for tools that don't mind if the brush scales mid stroke  */
  gboolean            handles_dynamic_transforming_brush;

  void (* set_brush)    (GimpBrushCore *core,
                         GimpBrush     *brush);
  void (* set_dynamics) (GimpBrushCore *core,
                         GimpDynamics  *brush);
};


GType  gimp_brush_core_get_type       (void) G_GNUC_CONST;

void   gimp_brush_core_set_brush      (GimpBrushCore            *core,
                                       GimpBrush                *brush);

void   gimp_brush_core_set_dynamics   (GimpBrushCore            *core,
                                       GimpDynamics             *dynamics);

void   gimp_brush_core_paste_canvas   (GimpBrushCore            *core,
                                       GimpDrawable             *drawable,
                                       const GimpCoords         *coords,
                                       gdouble                   brush_opacity,
                                       gdouble                   image_opacity,
                                       GimpLayerMode             paint_mode,
                                       GimpBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness,
                                       GimpPaintApplicationMode  mode);
void   gimp_brush_core_replace_canvas (GimpBrushCore            *core,
                                       GimpDrawable             *drawable,
                                       const GimpCoords         *coords,
                                       gdouble                   brush_opacity,
                                       gdouble                   image_opacity,
                                       GimpBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness,
                                       GimpPaintApplicationMode  mode);

void   gimp_brush_core_color_area_with_pixmap
                                      (GimpBrushCore            *core,
                                       GimpDrawable             *drawable,
                                       const GimpCoords         *coords,
                                       GeglBuffer               *area,
                                       gint                      area_x,
                                       gint                      area_y,
                                       gboolean                  apply_mask);

const GimpTempBuf * gimp_brush_core_get_brush_mask
                                      (GimpBrushCore            *core,
                                       const GimpCoords         *coords,
                                       GimpBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness);
const GimpTempBuf * gimp_brush_core_get_brush_pixmap
                                      (GimpBrushCore            *core);

void   gimp_brush_core_eval_transform_dynamics
                                      (GimpBrushCore            *core,
                                       GimpImage                *image,
                                       GimpPaintOptions         *paint_options,
                                       const GimpCoords         *coords);
void   gimp_brush_core_eval_transform_symmetry
                                      (GimpBrushCore            *core,
                                       GimpSymmetry             *symmetry,
                                       gint                      stroke);
