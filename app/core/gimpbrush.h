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

#ifndef __LIGMA_BRUSH_H__
#define __LIGMA_BRUSH_H__


#include "ligmadata.h"


#define LIGMA_TYPE_BRUSH            (ligma_brush_get_type ())
#define LIGMA_BRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH, LigmaBrush))
#define LIGMA_BRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH, LigmaBrushClass))
#define LIGMA_IS_BRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH))
#define LIGMA_IS_BRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH))
#define LIGMA_BRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH, LigmaBrushClass))


typedef struct _LigmaBrushPrivate LigmaBrushPrivate;
typedef struct _LigmaBrushClass   LigmaBrushClass;

struct _LigmaBrush
{
  LigmaData          parent_instance;

  LigmaBrushPrivate *priv;
};

struct _LigmaBrushClass
{
  LigmaDataClass  parent_class;

  /*  virtual functions  */
  void             (* begin_use)          (LigmaBrush        *brush);
  void             (* end_use)            (LigmaBrush        *brush);
  LigmaBrush      * (* select_brush)       (LigmaBrush        *brush,
                                           const LigmaCoords *last_coords,
                                           const LigmaCoords *current_coords);
  gboolean         (* want_null_motion)   (LigmaBrush        *brush,
                                           const LigmaCoords *last_coords,
                                           const LigmaCoords *current_coords);
  void             (* transform_size)     (LigmaBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gint             *width,
                                           gint             *height);
  LigmaTempBuf    * (* transform_mask)     (LigmaBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness);
  LigmaTempBuf    * (* transform_pixmap)   (LigmaBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness);
  LigmaBezierDesc * (* transform_boundary) (LigmaBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness,
                                           gint             *width,
                                           gint             *height);

  /*  signals  */
  void             (* spacing_changed)    (LigmaBrush        *brush);
};


GType                  ligma_brush_get_type           (void) G_GNUC_CONST;

LigmaData             * ligma_brush_new                (LigmaContext      *context,
                                                      const gchar      *name);
LigmaData             * ligma_brush_get_standard       (LigmaContext      *context);

void                   ligma_brush_begin_use          (LigmaBrush        *brush);
void                   ligma_brush_end_use            (LigmaBrush        *brush);

LigmaBrush            * ligma_brush_select_brush       (LigmaBrush        *brush,
                                                      const LigmaCoords *last_coords,
                                                      const LigmaCoords *current_coords);
gboolean               ligma_brush_want_null_motion   (LigmaBrush        *brush,
                                                      const LigmaCoords *last_coords,
                                                      const LigmaCoords *current_coords);

/* Gets width and height of a transformed mask of the brush, for
 * provided parameters.
 */
void                   ligma_brush_transform_size     (LigmaBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gint             *width,
                                                      gint             *height);
const LigmaTempBuf    * ligma_brush_transform_mask     (LigmaBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness);
const LigmaTempBuf    * ligma_brush_transform_pixmap   (LigmaBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness);
const LigmaBezierDesc * ligma_brush_transform_boundary (LigmaBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness,
                                                      gint             *width,
                                                      gint             *height);

LigmaTempBuf          * ligma_brush_get_mask           (LigmaBrush        *brush);
LigmaTempBuf          * ligma_brush_get_pixmap         (LigmaBrush        *brush);

gint                   ligma_brush_get_width          (LigmaBrush        *brush);
gint                   ligma_brush_get_height         (LigmaBrush        *brush);

gint                   ligma_brush_get_spacing        (LigmaBrush        *brush);
void                   ligma_brush_set_spacing        (LigmaBrush        *brush,
                                                      gint              spacing);

LigmaVector2            ligma_brush_get_x_axis         (LigmaBrush        *brush);
LigmaVector2            ligma_brush_get_y_axis         (LigmaBrush        *brush);

void                   ligma_brush_flush_blur_caches  (LigmaBrush        *brush);
gdouble                ligma_brush_get_blur_hardness  (LigmaBrush        *brush);

#endif /* __LIGMA_BRUSH_H__ */
