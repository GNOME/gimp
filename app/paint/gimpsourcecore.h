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

#ifndef __LIGMA_SOURCE_CORE_H__
#define __LIGMA_SOURCE_CORE_H__


#include "ligmabrushcore.h"


#define LIGMA_TYPE_SOURCE_CORE            (ligma_source_core_get_type ())
#define LIGMA_SOURCE_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SOURCE_CORE, LigmaSourceCore))
#define LIGMA_SOURCE_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SOURCE_CORE, LigmaSourceCoreClass))
#define LIGMA_IS_SOURCE_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SOURCE_CORE))
#define LIGMA_IS_SOURCE_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SOURCE_CORE))
#define LIGMA_SOURCE_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SOURCE_CORE, LigmaSourceCoreClass))


typedef struct _LigmaSourceCoreClass LigmaSourceCoreClass;

struct _LigmaSourceCore
{
  LigmaBrushCore  parent_instance;

  gboolean       set_source;

  gint           orig_src_x;
  gint           orig_src_y;

  gint           offset_x;
  gint           offset_y;
  gboolean       first_stroke;
};

struct _LigmaSourceCoreClass
{
  LigmaBrushCoreClass  parent_class;

  gboolean     (* use_source) (LigmaSourceCore    *source_core,
                               LigmaSourceOptions *options);

  GeglBuffer * (* get_source) (LigmaSourceCore    *source_core,
                               LigmaDrawable      *drawable,
                               LigmaPaintOptions  *paint_options,
                               gboolean           self_drawable,
                               LigmaPickable      *src_pickable,
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

  void         (*  motion)    (LigmaSourceCore    *source_core,
                               LigmaDrawable      *drawable,
                               LigmaPaintOptions  *paint_options,
                               const LigmaCoords  *coords,
                               GeglNode          *op,
                               gdouble            opacity,
                               LigmaPickable      *src_pickable,
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


GType    ligma_source_core_get_type   (void) G_GNUC_CONST;

gboolean ligma_source_core_use_source (LigmaSourceCore    *source_core,
                                      LigmaSourceOptions *options);

/* TEMP HACK */
void     ligma_source_core_motion     (LigmaSourceCore    *source_core,
                                      LigmaDrawable      *drawable,
                                      LigmaPaintOptions  *paint_options,
                                      gboolean           self_drawable,
                                      LigmaSymmetry      *sym);


#endif  /*  __LIGMA_SOURCE_CORE_H__  */
