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

#ifndef __GIMP_BRUSH_H__
#define __GIMP_BRUSH_H__


#include "gimpdata.h"


#define GIMP_TYPE_BRUSH            (gimp_brush_get_type ())
#define GIMP_BRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH, GimpBrush))
#define GIMP_BRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH, GimpBrushClass))
#define GIMP_IS_BRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH))
#define GIMP_IS_BRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH))
#define GIMP_BRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH, GimpBrushClass))


typedef struct _GimpBrushPrivate GimpBrushPrivate;
typedef struct _GimpBrushClass   GimpBrushClass;

struct _GimpBrush
{
  GimpData          parent_instance;

  GimpBrushPrivate *priv;
};

struct _GimpBrushClass
{
  GimpDataClass  parent_class;

  /*  virtual functions  */
  void             (* begin_use)          (GimpBrush        *brush);
  void             (* end_use)            (GimpBrush        *brush);
  GimpBrush      * (* select_brush)       (GimpBrush        *brush,
                                           const GimpCoords *last_coords,
                                           const GimpCoords *current_coords);
  gboolean         (* want_null_motion)   (GimpBrush        *brush,
                                           const GimpCoords *last_coords,
                                           const GimpCoords *current_coords);
  void             (* transform_size)     (GimpBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gint             *width,
                                           gint             *height);
  GimpTempBuf    * (* transform_mask)     (GimpBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness);
  GimpTempBuf    * (* transform_pixmap)   (GimpBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness);
  GimpBezierDesc * (* transform_boundary) (GimpBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness,
                                           gint             *width,
                                           gint             *height);

  /*  signals  */
  void             (* spacing_changed)    (GimpBrush        *brush);
};


GType                  gimp_brush_get_type           (void) G_GNUC_CONST;

GimpData             * gimp_brush_new                (GimpContext      *context,
                                                      const gchar      *name);
GimpData             * gimp_brush_get_standard       (GimpContext      *context);

void                   gimp_brush_begin_use          (GimpBrush        *brush);
void                   gimp_brush_end_use            (GimpBrush        *brush);

GimpBrush            * gimp_brush_select_brush       (GimpBrush        *brush,
                                                      const GimpCoords *last_coords,
                                                      const GimpCoords *current_coords);
gboolean               gimp_brush_want_null_motion   (GimpBrush        *brush,
                                                      const GimpCoords *last_coords,
                                                      const GimpCoords *current_coords);

/* Gets width and height of a transformed mask of the brush, for
 * provided parameters.
 */
void                   gimp_brush_transform_size     (GimpBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gint             *width,
                                                      gint             *height);
const GimpTempBuf    * gimp_brush_transform_mask     (GimpBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness);
const GimpTempBuf    * gimp_brush_transform_pixmap   (GimpBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness);
const GimpBezierDesc * gimp_brush_transform_boundary (GimpBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness,
                                                      gint             *width,
                                                      gint             *height);

GimpTempBuf          * gimp_brush_get_mask           (GimpBrush        *brush);
GimpTempBuf          * gimp_brush_get_pixmap         (GimpBrush        *brush);

gint                   gimp_brush_get_width          (GimpBrush        *brush);
gint                   gimp_brush_get_height         (GimpBrush        *brush);

gint                   gimp_brush_get_spacing        (GimpBrush        *brush);
void                   gimp_brush_set_spacing        (GimpBrush        *brush,
                                                      gint              spacing);

GimpVector2            gimp_brush_get_x_axis         (GimpBrush        *brush);
GimpVector2            gimp_brush_get_y_axis         (GimpBrush        *brush);

void                   gimp_brush_flush_blur_caches  (GimpBrush        *brush);
gdouble                gimp_brush_get_blur_hardness  (GimpBrush        *brush);

#endif /* __GIMP_BRUSH_H__ */
