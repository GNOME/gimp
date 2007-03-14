/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_BRUSH_H__
#define __GIMP_BRUSH_H__


#include "libgimpmath/gimpvector.h"
#include "gimpdata.h"


#define GIMP_TYPE_BRUSH            (gimp_brush_get_type ())
#define GIMP_BRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH, GimpBrush))
#define GIMP_BRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH, GimpBrushClass))
#define GIMP_IS_BRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH))
#define GIMP_IS_BRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH))
#define GIMP_BRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH, GimpBrushClass))


typedef struct _GimpBrushClass GimpBrushClass;

struct _GimpBrush
{
  GimpData      parent_instance;

  TempBuf      *mask;       /*  the actual mask                */
  TempBuf      *pixmap;     /*  optional pixmap data           */

  gint          spacing;    /*  brush's spacing                */
  GimpVector2   x_axis;     /*  for calculating brush spacing  */
  GimpVector2   y_axis;     /*  for calculating brush spacing  */
};

struct _GimpBrushClass
{
  GimpDataClass parent_class;

  /*  virtual functions  */
  GimpBrush * (* select_brush)     (GimpBrush  *brush,
                                    GimpCoords *last_coords,
                                    GimpCoords *cur_coords);
  gboolean    (* want_null_motion) (GimpBrush  *brush,
                                    GimpCoords *last_coords,
                                    GimpCoords *cur_coords);
  void        (* scale_size)       (GimpBrush  *brush,
                                    gdouble     scale,
                                    gint       *width,
                                    gint       *height);
  TempBuf   * (* scale_mask)       (GimpBrush  *brush,
                                    gdouble     scale);
  TempBuf   * (* scale_pixmap)     (GimpBrush  *brush,
                                    gdouble     scale);

  /*  signals  */
  void        (* spacing_changed)  (GimpBrush  *brush);
};


GType       gimp_brush_get_type         (void) G_GNUC_CONST;

GimpData  * gimp_brush_new              (const gchar      *name);
GimpData  * gimp_brush_get_standard     (void);

GimpBrush * gimp_brush_select_brush     (GimpBrush        *brush,
                                         GimpCoords       *last_coords,
                                         GimpCoords       *cur_coords);
gboolean    gimp_brush_want_null_motion (GimpBrush        *brush,
                                         GimpCoords       *last_coords,
                                         GimpCoords       *cur_coords);

/* Gets width and height of a scaled mask of the brush, for provided scale. */
void        gimp_brush_scale_size       (GimpBrush        *brush,
                                         gdouble           scale,
                                         gint             *width,
                                         gint             *height);
TempBuf   * gimp_brush_scale_mask       (GimpBrush        *brush,
                                         gdouble           scale);
TempBuf   * gimp_brush_scale_pixmap     (GimpBrush        *brush,
                                         gdouble           scale);

TempBuf   * gimp_brush_get_mask         (const GimpBrush  *brush);
TempBuf   * gimp_brush_get_pixmap       (const GimpBrush  *brush);

gint        gimp_brush_get_spacing      (const GimpBrush  *brush);
void        gimp_brush_set_spacing      (GimpBrush        *brush,
                                         gint              spacing);
void        gimp_brush_spacing_changed  (GimpBrush        *brush);


#endif /* __GIMP_BRUSH_H__ */
