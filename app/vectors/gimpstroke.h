/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstroke.h
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#ifndef __GIMP_STROKE_H__
#define __GIMP_STROKE_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_STROKE            (gimp_stroke_get_type ())
#define GIMP_STROKE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STROKE, GimpStroke))
#define GIMP_STROKE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STROKE, GimpStrokeClass))
#define GIMP_IS_STROKE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STROKE))
#define GIMP_IS_STROKE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_STROKE))
#define GIMP_STROKE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_STROKE, GimpStrokeClass))


struct _GimpStroke
{
  GimpObject  parent_instance;
               
  GList      *anchors;
               
  gboolean    closed;
};


struct _GimpStrokeClass
{
  GimpObjectClass  parent_class;

  void          (* changed)              (GimpStroke            *stroke);
  void          (* removed)              (GimpStroke            *stroke);

  GimpAnchor  * (* anchor_get)           (const GimpStroke      *stroke,
                                          const GimpCoords      *coord);
  gdouble       (* nearest_point_get)    (const GimpStroke      *stroke,
                                          const GimpCoords      *coord,
                                          const gdouble          precision,
                                          GimpCoords            *ret_point,
                                          GimpAnchor           **ret_segment_start,
                                          gdouble               *ret_pos);
  GimpAnchor  * (* anchor_get_next)      (const GimpStroke      *stroke,
                                          const GimpAnchor      *prev);
  void          (* anchor_select)        (GimpStroke            *stroke,
                                          GimpAnchor            *anchor,
                                          gboolean               exclusive);
  void          (* anchor_move_relative) (GimpStroke            *stroke,
                                          GimpAnchor            *anchor,
                                          const GimpCoords      *deltacoord,
                                          GimpAnchorFeatureType  feature);
  void          (* anchor_move_absolute) (GimpStroke            *stroke,
                                          GimpAnchor            *anchor,
                                          const GimpCoords      *coord,
                                          GimpAnchorFeatureType  feature);
  void          (* anchor_convert)       (GimpStroke            *stroke,
                                          GimpAnchor            *anchor,
                                          GimpAnchorFeatureType  feature);
  void          (* anchor_delete)        (GimpStroke            *stroke,
                                          GimpAnchor            *anchor);

  GimpStroke  * (* open)                 (GimpStroke            *stroke,
                                          GimpAnchor            *end_anchor);
  gboolean      (* anchor_is_insertable) (GimpStroke            *stroke,
                                          GimpAnchor            *predec,
                                          gdouble                position);
  GimpAnchor  * (* anchor_insert)        (GimpStroke            *stroke,
                                          GimpAnchor            *predec,
                                          gdouble                position);
  gboolean      (* is_extendable)        (GimpStroke            *stroke,
                                          GimpAnchor            *neighbor);
  GimpAnchor  * (* extend)               (GimpStroke            *stroke,
                                          const GimpCoords      *coords,
                                          GimpAnchor            *neighbor,
                                          GimpVectorExtendMode   extend_mode);

  gboolean      (* is_empty)             (const GimpStroke      *stroke);
  gdouble       (* get_length)           (const GimpStroke      *stroke);
  gdouble       (* get_distance)         (const GimpStroke      *stroke,
                                          const GimpCoords      *coord);

  GArray      * (* interpolate)          (const GimpStroke      *stroke,
                                          const gdouble          precision,
                                          gboolean              *ret_closed);

  GimpStroke  * (* duplicate)            (const GimpStroke      *stroke);

  GimpStroke  * (* make_bezier)          (const GimpStroke      *stroke);

  void          (* translate)            (GimpStroke            *stroke,
                                          gdouble                offset_x,
                                          gdouble                offset_y);

  void          (* scale)                (GimpStroke            *stroke,
                                          gdouble                scale_x,
                                          gdouble                scale_y,
                                          gint                   offset_x,
                                          gint                   offset_y);

  void          (* resize)               (GimpStroke            *stroke,
                                          gint                   new_width,
                                          gint                   new_heigth,
                                          gint                   offset_x,
                                          gint                   offset_y);

  void          (* flip)                 (GimpStroke            *stroke,
                                          GimpOrientationType    flip_type,
                                          gdouble                axis,
                                          gboolean               clip_result);

  void          (* rotate)               (GimpStroke            *stroke,
                                          GimpRotationType       rotate_type,
                                          gdouble                center_x,
                                          gdouble                center_y,
                                          gboolean               clip_result);

  void          (* transform)            (GimpStroke            *stroke,
                                          const GimpMatrix3     *matrix,
                                          GimpTransformDirection direction,
                                          GimpInterpolationType  interp_type,
                                          gboolean               clip_result,
                                          GimpProgressFunc       progress_callback,
                                          gpointer               progress_data);

  GList       * (* get_draw_anchors)     (const GimpStroke      *stroke);
  GList       * (* get_draw_controls)    (const GimpStroke      *stroke);
  GArray      * (* get_draw_lines)       (const GimpStroke      *stroke);
};


GType        gimp_stroke_get_type             (void) G_GNUC_CONST;


/* accessing / modifying the anchors */

GimpAnchor * gimp_stroke_anchor_get           (const GimpStroke      *stroke,
                                               const GimpCoords      *coord);

gdouble      gimp_stroke_nearest_point_get    (const GimpStroke      *stroke,
                                               const GimpCoords      *coord,
                                               const gdouble          precision,
                                               GimpCoords            *ret_point,
                                               GimpAnchor           **ret_segment_start,
                                               gdouble               *ret_pos);
                                               

/* prev == NULL: "first" anchor */                                  
GimpAnchor * gimp_stroke_anchor_get_next      (const GimpStroke      *stroke,
                                               const GimpAnchor      *prev);

void         gimp_stroke_anchor_select        (GimpStroke            *stroke,
                                               GimpAnchor            *anchor,
                                               gboolean               exclusive);

/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void         gimp_stroke_anchor_move_relative (GimpStroke            *stroke,
                                               GimpAnchor            *anchor,
                                               const GimpCoords      *delta,
                                               GimpAnchorFeatureType  feature);
void         gimp_stroke_anchor_move_absolute (GimpStroke            *stroke,
                                               GimpAnchor            *anchor,
                                               const GimpCoords      *coord,
                                               GimpAnchorFeatureType  feature);

void         gimp_stroke_close                (GimpStroke            *stroke);

void         gimp_stroke_anchor_convert       (GimpStroke            *stroke,
                                               GimpAnchor            *anchor,
                                               GimpAnchorFeatureType  feature);

void         gimp_stroke_anchor_delete        (GimpStroke            *stroke,
                                               GimpAnchor            *anchor);

GimpStroke * gimp_stroke_open                 (GimpStroke            *stroke,
                                               GimpAnchor            *end_anchor);
gboolean     gimp_stroke_anchor_is_insertable (GimpStroke            *stroke,
                                               GimpAnchor            *predec,
                                               gdouble                position);
GimpAnchor  * gimp_stroke_anchor_insert       (GimpStroke            *stroke,
                                               GimpAnchor            *predec,
                                               gdouble                position);

gboolean     gimp_stroke_is_extendable        (GimpStroke            *stroke,
                                               GimpAnchor            *neighbor);

GimpAnchor * gimp_stroke_extend               (GimpStroke            *stroke,
                                               const GimpCoords      *coords,
                                               GimpAnchor            *neighbor,
                                               GimpVectorExtendMode   extend_mode);
                                          
gboolean     gimp_stroke_is_empty             (const GimpStroke      *stroke);

/* accessing the shape of the curve */

gdouble      gimp_stroke_get_length           (const GimpStroke      *stroke);
gdouble      gimp_stroke_get_distance         (const GimpStroke      *stroke,
                                               const GimpCoords      *coord);

/* returns an array of valid coordinates */                       
GArray     * gimp_stroke_interpolate          (const GimpStroke      *stroke,
                                               gdouble                precision,
                                               gboolean              *closed);

GimpStroke * gimp_stroke_duplicate            (const GimpStroke      *stroke);

/* creates a bezier approximation. */
GimpStroke * gimp_stroke_make_bezier          (const GimpStroke      *stroke);

void         gimp_stroke_translate   (GimpStroke             *stroke,
                                      gdouble                 offset_x,
                                      gdouble                 offset_y);
void         gimp_stroke_scale       (GimpStroke             *stroke,
                                      gdouble                 scale_x,
                                      gdouble                 scale_y,
                                      gint                    offset_x,
                                      gint                    offset_y);
void         gimp_stroke_resize      (GimpStroke             *stroke,
                                      gint                    new_width,
                                      gint                    new_height,
                                      gint                    offset_x,
                                      gint                    offset_y);
void         gimp_stroke_flip        (GimpStroke             *stroke,
                                      GimpOrientationType     flip_type,
                                      gdouble                 axis,
                                      gboolean                clip_result);
void         gimp_stroke_rotate      (GimpStroke             *stroke,
                                      GimpRotationType        rotate_type,
                                      gdouble                 center_x,
                                      gdouble                 center_y,
                                      gboolean                clip_result);
void         gimp_stroke_transform   (GimpStroke             *stroke,
                                      const GimpMatrix3      *matrix,
                                      GimpTransformDirection  direction,
                                      GimpInterpolationType   interp_type,
                                      gboolean                clip_result,
                                      GimpProgressFunc        progress_callback,
                                      gpointer                progress_data);


GList      * gimp_stroke_get_draw_anchors     (const GimpStroke      *stroke);
GList      * gimp_stroke_get_draw_controls    (const GimpStroke      *stroke);
GArray     * gimp_stroke_get_draw_lines       (const GimpStroke      *stroke);


#endif /* __GIMP_STROKE_H__ */
