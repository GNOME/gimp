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

#ifndef __LIGMA_PAINT_CORE_H__
#define __LIGMA_PAINT_CORE_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_PAINT_CORE            (ligma_paint_core_get_type ())
#define LIGMA_PAINT_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PAINT_CORE, LigmaPaintCore))
#define LIGMA_PAINT_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PAINT_CORE, LigmaPaintCoreClass))
#define LIGMA_IS_PAINT_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PAINT_CORE))
#define LIGMA_IS_PAINT_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PAINT_CORE))
#define LIGMA_PAINT_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PAINT_CORE, LigmaPaintCoreClass))


typedef struct _LigmaPaintCoreClass LigmaPaintCoreClass;

struct _LigmaPaintCore
{
  LigmaObject      parent_instance;

  gint            ID;                /*  unique instance ID                  */

  gchar          *undo_desc;         /*  undo description                    */

  gboolean        show_all;          /*  whether working in show-all mode    */

  LigmaCoords      start_coords;      /*  the last stroke's endpoint for undo */

  LigmaCoords      cur_coords;        /*  current coords                      */
  LigmaCoords      last_coords;       /*  last coords                         */

  LigmaVector2     last_paint;        /*  last point that was painted         */

  gdouble         distance;          /*  distance traveled by brush          */
  gdouble         pixel_dist;        /*  distance in pixels                  */

  gint            x1, y1;            /*  undo extents in image coords        */
  gint            x2, y2;            /*  undo extents in image coords        */

  gboolean        use_saved_proj;    /*  keep the unmodified proj around     */

  LigmaPickable   *image_pickable;    /*  the image pickable                  */

  GHashTable     *undo_buffers;      /*  pixels which have been modified     */
  GeglBuffer     *saved_proj_buffer; /*  proj tiles which have been modified */
  GeglBuffer     *canvas_buffer;     /*  the buffer to paint the mask to     */
  GeglBuffer     *paint_buffer;      /*  the buffer to paint pixels to       */
  gint            paint_buffer_x;
  gint            paint_buffer_y;

  GeglBuffer     *mask_buffer;       /*  the target drawable's mask          */

  GHashTable     *applicators;

  GArray         *stroke_buffer;
};

struct _LigmaPaintCoreClass
{
  LigmaObjectClass  parent_class;

  /*  virtual functions  */
  gboolean     (* start)            (LigmaPaintCore    *core,
                                     GList            *drawables,
                                     LigmaPaintOptions *paint_options,
                                     const LigmaCoords *coords,
                                     GError          **error);

  gboolean     (* pre_paint)        (LigmaPaintCore    *core,
                                     GList            *drawables,
                                     LigmaPaintOptions *paint_options,
                                     LigmaPaintState    paint_state,
                                     guint32           time);
  void         (* paint)            (LigmaPaintCore    *core,
                                     GList            *drawables,
                                     LigmaPaintOptions *paint_options,
                                     LigmaSymmetry     *sym,
                                     LigmaPaintState    paint_state,
                                     guint32           time);
  void         (* post_paint)       (LigmaPaintCore    *core,
                                     GList            *drawables,
                                     LigmaPaintOptions *paint_options,
                                     LigmaPaintState    paint_state,
                                     guint32           time);

  void         (* interpolate)      (LigmaPaintCore    *core,
                                     GList            *drawables,
                                     LigmaPaintOptions *paint_options,
                                     guint32           time);

  GeglBuffer * (* get_paint_buffer) (LigmaPaintCore    *core,
                                     LigmaDrawable     *drawable,
                                     LigmaPaintOptions *paint_options,
                                     LigmaLayerMode     paint_mode,
                                     const LigmaCoords *coords,
                                     gint             *paint_buffer_x,
                                     gint             *paint_buffer_y,
                                     gint             *paint_width,
                                     gint             *paint_height);

  LigmaUndo   * (* push_undo)        (LigmaPaintCore    *core,
                                     LigmaImage        *image,
                                     const gchar      *undo_desc);
};


GType     ligma_paint_core_get_type                  (void) G_GNUC_CONST;

void      ligma_paint_core_paint                     (LigmaPaintCore    *core,
                                                     GList            *drawables,
                                                     LigmaPaintOptions *paint_options,
                                                     LigmaPaintState    state,
                                                     guint32           time);

gboolean  ligma_paint_core_start                     (LigmaPaintCore    *core,
                                                     GList            *drawables,
                                                     LigmaPaintOptions *paint_options,
                                                     const LigmaCoords *coords,
                                                     GError          **error);
void      ligma_paint_core_finish                    (LigmaPaintCore    *core,
                                                     GList            *drawables,
                                                     gboolean          push_undo);
void      ligma_paint_core_cancel                    (LigmaPaintCore    *core,
                                                     GList            *drawables);
void      ligma_paint_core_cleanup                   (LigmaPaintCore    *core);

void      ligma_paint_core_interpolate               (LigmaPaintCore    *core,
                                                     GList            *drawables,
                                                     LigmaPaintOptions *paint_options,
                                                     const LigmaCoords *coords,
                                                     guint32           time);

void      ligma_paint_core_set_show_all              (LigmaPaintCore    *core,
                                                     gboolean          show_all);
gboolean  ligma_paint_core_get_show_all              (LigmaPaintCore    *core);

void      ligma_paint_core_set_current_coords        (LigmaPaintCore    *core,
                                                     const LigmaCoords *coords);
void      ligma_paint_core_get_current_coords        (LigmaPaintCore    *core,
                                                     LigmaCoords       *coords);

void      ligma_paint_core_set_last_coords           (LigmaPaintCore    *core,
                                                     const LigmaCoords *coords);
void      ligma_paint_core_get_last_coords           (LigmaPaintCore    *core,
                                                     LigmaCoords       *coords);

void      ligma_paint_core_round_line                (LigmaPaintCore    *core,
                                                     LigmaPaintOptions *options,
                                                     gboolean          constrain_15_degrees,
                                                     gdouble           constrain_offset_angle,
                                                     gdouble           constrain_xres,
                                                     gdouble           constrain_yres);


/*  protected functions  */

GeglBuffer * ligma_paint_core_get_paint_buffer       (LigmaPaintCore    *core,
                                                     LigmaDrawable     *drawable,
                                                     LigmaPaintOptions *options,
                                                     LigmaLayerMode     paint_mode,
                                                     const LigmaCoords *coords,
                                                     gint             *paint_buffer_x,
                                                     gint             *paint_buffer_y,
                                                     gint             *paint_width,
                                                     gint             *paint_height);

LigmaPickable * ligma_paint_core_get_image_pickable   (LigmaPaintCore    *core);

GeglBuffer * ligma_paint_core_get_orig_image         (LigmaPaintCore    *core,
                                                     LigmaDrawable     *drawable);
GeglBuffer * ligma_paint_core_get_orig_proj          (LigmaPaintCore    *core);

void      ligma_paint_core_paste             (LigmaPaintCore            *core,
                                             const LigmaTempBuf        *paint_mask,
                                             gint                      paint_mask_offset_x,
                                             gint                      paint_mask_offset_y,
                                             LigmaDrawable             *drawable,
                                             gdouble                   paint_opacity,
                                             gdouble                   image_opacity,
                                             LigmaLayerMode             paint_mode,
                                             LigmaPaintApplicationMode  mode);

void      ligma_paint_core_replace           (LigmaPaintCore            *core,
                                             const LigmaTempBuf        *paint_mask,
                                             gint                      paint_mask_offset_x,
                                             gint                      paint_mask_offset_y,
                                             LigmaDrawable             *drawable,
                                             gdouble                   paint_opacity,
                                             gdouble                   image_opacity,
                                             LigmaPaintApplicationMode  mode);

void      ligma_paint_core_smooth_coords             (LigmaPaintCore    *core,
                                                     LigmaPaintOptions *paint_options,
                                                     LigmaCoords       *coords);


#endif  /*  __LIGMA_PAINT_CORE_H__  */
