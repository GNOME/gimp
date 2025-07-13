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

#include "core/gimpobject.h"


#define GIMP_TYPE_PAINT_CORE            (gimp_paint_core_get_type ())
#define GIMP_PAINT_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINT_CORE, GimpPaintCore))
#define GIMP_PAINT_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_CORE, GimpPaintCoreClass))
#define GIMP_IS_PAINT_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINT_CORE))
#define GIMP_IS_PAINT_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_CORE))
#define GIMP_PAINT_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINT_CORE, GimpPaintCoreClass))


typedef struct _GimpPaintCoreClass GimpPaintCoreClass;

struct _GimpPaintCore
{
  GimpObject      parent_instance;

  gint            ID;                /*  unique instance ID                  */

  gchar          *undo_desc;         /*  undo description                    */

  gboolean        show_all;          /*  whether working in show-all mode    */

  GimpCoords      start_coords;      /*  the last stroke's endpoint for undo */

  GimpCoords      cur_coords;        /*  current coords                      */
  GimpCoords      last_coords;       /*  last coords                         */

  GimpVector2     last_paint;        /*  last point that was painted         */

  gdouble         distance;          /*  distance traveled by brush          */
  gdouble         pixel_dist;        /*  distance in pixels                  */

  gint            x1, y1;            /*  undo extents in image coords        */
  gint            x2, y2;            /*  undo extents in image coords        */

  gboolean        use_saved_proj;    /*  keep the unmodified proj around     */

  GimpPickable   *image_pickable;    /*  the image pickable                  */

  GHashTable     *undo_buffers;      /*  pixels which have been modified     */
  GeglBuffer     *saved_proj_buffer; /*  proj tiles which have been modified */
  GeglBuffer     *canvas_buffer;     /*  the buffer to paint the mask to     */
  GeglBuffer     *paint_buffer;      /*  the buffer to paint pixels to       */
  gint            paint_buffer_x;
  gint            paint_buffer_y;
  GHashTable     *original_bounds;   /* the original bounds of drawables     */

  GeglBuffer     *mask_buffer;       /*  the target drawable's mask          */

  GHashTable     *applicators;

  GArray         *stroke_buffer;

  GimpSymmetry   *sym;
  GimpPaintLockBlinkState
                  lock_blink_state;
};

struct _GimpPaintCoreClass
{
  GimpObjectClass  parent_class;

  /*  virtual functions  */
  gboolean     (* start)            (GimpPaintCore    *core,
                                     GList            *drawables,
                                     GimpPaintOptions *paint_options,
                                     const GimpCoords *coords,
                                     GError          **error);

  gboolean     (* pre_paint)        (GimpPaintCore    *core,
                                     GList            *drawables,
                                     GimpPaintOptions *paint_options,
                                     GimpPaintState    paint_state,
                                     guint32           time);
  void         (* paint)            (GimpPaintCore    *core,
                                     GList            *drawables,
                                     GimpPaintOptions *paint_options,
                                     GimpSymmetry     *sym,
                                     GimpPaintState    paint_state,
                                     guint32           time);
  void         (* post_paint)       (GimpPaintCore    *core,
                                     GList            *drawables,
                                     GimpPaintOptions *paint_options,
                                     GimpPaintState    paint_state,
                                     guint32           time);

  void         (* interpolate)      (GimpPaintCore    *core,
                                     GList            *drawables,
                                     GimpPaintOptions *paint_options,
                                     guint32           time);

  GeglBuffer * (* get_paint_buffer) (GimpPaintCore    *core,
                                     GimpDrawable     *drawable,
                                     GimpPaintOptions *paint_options,
                                     GimpLayerMode     paint_mode,
                                     const GimpCoords *coords,
                                     gint             *paint_buffer_x,
                                     gint             *paint_buffer_y,
                                     gint             *paint_width,
                                     gint             *paint_height);

  GimpUndo   * (* push_undo)        (GimpPaintCore    *core,
                                     GimpImage        *image,
                                     const gchar      *undo_desc);
};


GType     gimp_paint_core_get_type                  (void) G_GNUC_CONST;

void      gimp_paint_core_paint                     (GimpPaintCore    *core,
                                                     GList            *drawables,
                                                     GimpPaintOptions *paint_options,
                                                     GimpPaintState    state,
                                                     guint32           time);

gboolean  gimp_paint_core_start                     (GimpPaintCore    *core,
                                                     GList            *drawables,
                                                     GimpPaintOptions *paint_options,
                                                     const GimpCoords *coords,
                                                     GError          **error);
void      gimp_paint_core_finish                    (GimpPaintCore    *core,
                                                     GList            *drawables,
                                                     gboolean          push_undo);
void      gimp_paint_core_cancel                    (GimpPaintCore    *core,
                                                     GList            *drawables);
void      gimp_paint_core_cleanup                   (GimpPaintCore    *core);

void      gimp_paint_core_interpolate               (GimpPaintCore    *core,
                                                     GList            *drawables,
                                                     GimpPaintOptions *paint_options,
                                                     const GimpCoords *coords,
                                                     guint32           time);

void      gimp_paint_core_set_show_all              (GimpPaintCore    *core,
                                                     gboolean          show_all);
gboolean  gimp_paint_core_get_show_all              (GimpPaintCore    *core);

gboolean  gimp_paint_core_expand_drawable           (GimpPaintCore    *paint_core,
                                                     GimpDrawable     *drawable,
                                                     GimpPaintOptions *paint_options,
                                                     gint              x1,
                                                     gint              x2,
                                                     gint              y1,
                                                     gint              y2,
                                                     gint             *x,
                                                     gint             *y);

void      gimp_paint_core_set_current_coords        (GimpPaintCore    *core,
                                                     const GimpCoords *coords);
void      gimp_paint_core_get_current_coords        (GimpPaintCore    *core,
                                                     GimpCoords       *coords);

void      gimp_paint_core_set_last_coords           (GimpPaintCore    *core,
                                                     const GimpCoords *coords);
void      gimp_paint_core_get_last_coords           (GimpPaintCore    *core,
                                                     GimpCoords       *coords);

void      gimp_paint_core_round_line                (GimpPaintCore    *core,
                                                     GimpPaintOptions *options,
                                                     gboolean          constrain_15_degrees,
                                                     gdouble           constrain_offset_angle,
                                                     gdouble           constrain_xres,
                                                     gdouble           constrain_yres);


/*  protected functions  */

GeglBuffer * gimp_paint_core_get_paint_buffer       (GimpPaintCore    *core,
                                                     GimpDrawable     *drawable,
                                                     GimpPaintOptions *options,
                                                     GimpLayerMode     paint_mode,
                                                     const GimpCoords *coords,
                                                     gint             *paint_buffer_x,
                                                     gint             *paint_buffer_y,
                                                     gint             *paint_width,
                                                     gint             *paint_height);

GimpPickable * gimp_paint_core_get_image_pickable   (GimpPaintCore    *core);

GeglBuffer * gimp_paint_core_get_orig_image         (GimpPaintCore    *core,
                                                     GimpDrawable     *drawable);
GeglBuffer * gimp_paint_core_get_orig_proj          (GimpPaintCore    *core);

void      gimp_paint_core_paste             (GimpPaintCore            *core,
                                             const GimpTempBuf        *paint_mask,
                                             gint                      paint_mask_offset_x,
                                             gint                      paint_mask_offset_y,
                                             GimpDrawable             *drawable,
                                             gdouble                   paint_opacity,
                                             gdouble                   image_opacity,
                                             GimpLayerMode             paint_mode,
                                             GimpPaintApplicationMode  mode);

void      gimp_paint_core_replace           (GimpPaintCore            *core,
                                             const GimpTempBuf        *paint_mask,
                                             gint                      paint_mask_offset_x,
                                             gint                      paint_mask_offset_y,
                                             GimpDrawable             *drawable,
                                             gdouble                   paint_opacity,
                                             gdouble                   image_opacity,
                                             GimpPaintApplicationMode  mode);

void      gimp_paint_core_smooth_coords             (GimpPaintCore    *core,
                                                     GimpPaintOptions *paint_options,
                                                     GimpCoords       *coords);

