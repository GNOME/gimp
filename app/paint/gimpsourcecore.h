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

#ifndef __GIMP_SOURCE_CORE_H__
#define __GIMP_SOURCE_CORE_H__


#include "gimpbrushcore.h"


#define GIMP_TYPE_SOURCE_CORE            (gimp_source_core_get_type ())
#define GIMP_SOURCE_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SOURCE_CORE, GimpSourceCore))
#define GIMP_SOURCE_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SOURCE_CORE, GimpSourceCoreClass))
#define GIMP_IS_SOURCE_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SOURCE_CORE))
#define GIMP_IS_SOURCE_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SOURCE_CORE))
#define GIMP_SOURCE_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SOURCE_CORE, GimpSourceCoreClass))


typedef struct _GimpSourceCoreClass GimpSourceCoreClass;

struct _GimpSourceCore
{
  GimpBrushCore  parent_instance;

  gboolean       set_source;

  gint           orig_src_x;
  gint           orig_src_y;

  gint           offset_x;
  gint           offset_y;
  gboolean       first_stroke;
};

struct _GimpSourceCoreClass
{
  GimpBrushCoreClass  parent_class;

  gboolean     (* use_source) (GimpSourceCore    *source_core,
                               GimpSourceOptions *options);

  GeglBuffer * (* get_source) (GimpSourceCore    *source_core,
                               GimpDrawable      *drawable,
                               GimpPaintOptions  *paint_options,
                               gboolean           self_drawable,
                               GimpPickable      *src_pickable,
                               gint               src_offset_x,
                               gint               src_offset_y,
                               GeglBuffer        *paint_buffer,
                               gint               paint_buffer_x,
                               gint               paint_buffer_y,
                               /* offsets *into* the paint_buffer: */
                               gint              *paint_area_offset_x,
                               gint              *paint_area_offset_y,
                               gint              *paint_area_width,
                               gint              *paint_area_height,
                               GeglRectangle     *src_rect);

  void         (*  motion)    (GimpSourceCore    *source_core,
                               GimpDrawable      *drawable,
                               GimpPaintOptions  *paint_options,
                               const GimpCoords  *coords,
                               GeglNode          *op,
                               gdouble            opacity,
                               GimpPickable      *src_pickable,
                               GeglBuffer        *src_buffer,
                               GeglRectangle     *src_rect,
                               gint               src_offset_x,
                               gint               src_offset_y,
                               GeglBuffer        *paint_buffer,
                               gint               paint_buffer_x,
                               gint               paint_buffer_y,
                               /* offsets *into* the paint_buffer: */
                               gint               paint_area_offset_x,
                               gint               paint_area_offset_y,
                               gint               paint_area_width,
                               gint               paint_area_height);
};


GType    gimp_source_core_get_type   (void) G_GNUC_CONST;

gboolean gimp_source_core_use_source (GimpSourceCore    *source_core,
                                      GimpSourceOptions *options);

/* TEMP HACK */
void     gimp_source_core_motion     (GimpSourceCore    *source_core,
                                      GimpDrawable      *drawable,
                                      GimpPaintOptions  *paint_options,
                                      gboolean           self_drawable,
                                      GimpSymmetry      *sym);


#endif  /*  __GIMP_SOURCE_CORE_H__  */
