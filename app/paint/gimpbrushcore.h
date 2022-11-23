/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_BRUSH_CORE_H__
#define __LIGMA_BRUSH_CORE_H__


#include "ligmapaintcore.h"


#define BRUSH_CORE_SUBSAMPLE        4
#define BRUSH_CORE_SOLID_SUBSAMPLE  2
#define BRUSH_CORE_JITTER_LUTSIZE   360


#define LIGMA_TYPE_BRUSH_CORE            (ligma_brush_core_get_type ())
#define LIGMA_BRUSH_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH_CORE, LigmaBrushCore))
#define LIGMA_BRUSH_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH_CORE, LigmaBrushCoreClass))
#define LIGMA_IS_BRUSH_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH_CORE))
#define LIGMA_IS_BRUSH_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH_CORE))
#define LIGMA_BRUSH_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH_CORE, LigmaBrushCoreClass))


typedef struct _LigmaBrushCoreClass LigmaBrushCoreClass;

struct _LigmaBrushCore
{
  LigmaPaintCore      parent_instance;

  LigmaBrush         *main_brush;
  LigmaBrush         *brush;
  LigmaDynamics      *dynamics;
  gdouble            spacing;
  gdouble            scale;
  gdouble            aspect_ratio;
  gdouble            angle;
  gboolean           reflect;
  gdouble            hardness;

  gdouble            symmetry_angle;
  gboolean           symmetry_reflect;

  /*  brush buffers  */
  LigmaTempBuf       *pressure_brush;

  LigmaTempBuf       *solid_brushes[BRUSH_CORE_SOLID_SUBSAMPLE][BRUSH_CORE_SOLID_SUBSAMPLE];
  const LigmaTempBuf *last_solid_brush_mask;
  gboolean           solid_cache_invalid;

  const LigmaTempBuf *transform_brush;
  const LigmaTempBuf *transform_pixmap;

  LigmaTempBuf       *subsample_brushes[BRUSH_CORE_SUBSAMPLE + 1][BRUSH_CORE_SUBSAMPLE + 1];
  const LigmaTempBuf *last_subsample_brush_mask;
  gboolean           subsample_cache_invalid;

  gdouble            jitter;
  gdouble            jitter_lut_x[BRUSH_CORE_JITTER_LUTSIZE];
  gdouble            jitter_lut_y[BRUSH_CORE_JITTER_LUTSIZE];

  GRand             *rand;
};

struct _LigmaBrushCoreClass
{
  LigmaPaintCoreClass  parent_class;

  /*  Set for tools that don't mind if the brush changes while painting  */
  gboolean            handles_changing_brush;

  /*  Set for tools that don't mind if the brush scales while painting  */
  gboolean            handles_transforming_brush;

  /*  Set for tools that don't mind if the brush scales mid stroke  */
  gboolean            handles_dynamic_transforming_brush;

  void (* set_brush)    (LigmaBrushCore *core,
                         LigmaBrush     *brush);
  void (* set_dynamics) (LigmaBrushCore *core,
                         LigmaDynamics  *brush);
};


GType  ligma_brush_core_get_type       (void) G_GNUC_CONST;

void   ligma_brush_core_set_brush      (LigmaBrushCore            *core,
                                       LigmaBrush                *brush);

void   ligma_brush_core_set_dynamics   (LigmaBrushCore            *core,
                                       LigmaDynamics             *dynamics);

void   ligma_brush_core_paste_canvas   (LigmaBrushCore            *core,
                                       LigmaDrawable             *drawable,
                                       const LigmaCoords         *coords,
                                       gdouble                   brush_opacity,
                                       gdouble                   image_opacity,
                                       LigmaLayerMode             paint_mode,
                                       LigmaBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness,
                                       LigmaPaintApplicationMode  mode);
void   ligma_brush_core_replace_canvas (LigmaBrushCore            *core,
                                       LigmaDrawable             *drawable,
                                       const LigmaCoords         *coords,
                                       gdouble                   brush_opacity,
                                       gdouble                   image_opacity,
                                       LigmaBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness,
                                       LigmaPaintApplicationMode  mode);

void   ligma_brush_core_color_area_with_pixmap
                                      (LigmaBrushCore            *core,
                                       LigmaDrawable             *drawable,
                                       const LigmaCoords         *coords,
                                       GeglBuffer               *area,
                                       gint                      area_x,
                                       gint                      area_y,
                                       gboolean                  apply_mask);

const LigmaTempBuf * ligma_brush_core_get_brush_mask
                                      (LigmaBrushCore            *core,
                                       const LigmaCoords         *coords,
                                       LigmaBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness);
const LigmaTempBuf * ligma_brush_core_get_brush_pixmap
                                      (LigmaBrushCore            *core);

void   ligma_brush_core_eval_transform_dynamics
                                      (LigmaBrushCore            *core,
                                       LigmaImage                *image,
                                       LigmaPaintOptions         *paint_options,
                                       const LigmaCoords         *coords);
void   ligma_brush_core_eval_transform_symmetry
                                      (LigmaBrushCore            *core,
                                       LigmaSymmetry             *symmetry,
                                       gint                      stroke);


#endif  /*  __LIGMA_BRUSH_CORE_H__  */
