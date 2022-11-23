/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmastroke.h
 * Copyright (C) 2002 Simon Budig  <simon@ligma.org>
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

#ifndef __LIGMA_STROKE_H__
#define __LIGMA_STROKE_H__

#include "core/ligmaobject.h"


#define LIGMA_TYPE_STROKE            (ligma_stroke_get_type ())
#define LIGMA_STROKE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_STROKE, LigmaStroke))
#define LIGMA_STROKE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_STROKE, LigmaStrokeClass))
#define LIGMA_IS_STROKE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_STROKE))
#define LIGMA_IS_STROKE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_STROKE))
#define LIGMA_STROKE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_STROKE, LigmaStrokeClass))


typedef struct _LigmaStrokeClass LigmaStrokeClass;

struct _LigmaStroke
{
  LigmaObject  parent_instance;
  gint        id;

  GQueue     *anchors;

  gboolean    closed;
};

struct _LigmaStrokeClass
{
  LigmaObjectClass  parent_class;

  void          (* changed)              (LigmaStroke            *stroke);
  void          (* removed)              (LigmaStroke            *stroke);

  LigmaAnchor  * (* anchor_get)           (LigmaStroke            *stroke,
                                          const LigmaCoords      *coord);
  gdouble       (* nearest_point_get)    (LigmaStroke            *stroke,
                                          const LigmaCoords      *coord,
                                          gdouble                precision,
                                          LigmaCoords            *ret_point,
                                          LigmaAnchor           **ret_segment_start,
                                          LigmaAnchor           **ret_segment_end,
                                          gdouble               *ret_pos);
  gdouble       (* nearest_tangent_get)  (LigmaStroke            *stroke,
                                          const LigmaCoords      *coord1,
                                          const LigmaCoords      *coord2,
                                          gdouble                precision,
                                          LigmaCoords            *nearest,
                                          LigmaAnchor           **ret_segment_start,
                                          LigmaAnchor           **ret_segment_end,
                                          gdouble               *ret_pos);
  gdouble       (* nearest_intersection_get)
                                         (LigmaStroke            *stroke,
                                          const LigmaCoords      *coord1,
                                          const LigmaCoords      *direction,
                                          gdouble                precision,
                                          LigmaCoords            *nearest,
                                          LigmaAnchor           **ret_segment_start,
                                          LigmaAnchor           **ret_segment_end,
                                          gdouble               *ret_pos);
  LigmaAnchor  * (* anchor_get_next)      (LigmaStroke            *stroke,
                                          const LigmaAnchor      *prev);
  void          (* anchor_select)        (LigmaStroke            *stroke,
                                          LigmaAnchor            *anchor,
                                          gboolean               selected,
                                          gboolean               exclusive);
  void          (* anchor_move_relative) (LigmaStroke            *stroke,
                                          LigmaAnchor            *anchor,
                                          const LigmaCoords      *deltacoord,
                                          LigmaAnchorFeatureType  feature);
  void          (* anchor_move_absolute) (LigmaStroke            *stroke,
                                          LigmaAnchor            *anchor,
                                          const LigmaCoords      *coord,
                                          LigmaAnchorFeatureType  feature);
  void          (* anchor_convert)       (LigmaStroke            *stroke,
                                          LigmaAnchor            *anchor,
                                          LigmaAnchorFeatureType  feature);
  void          (* anchor_delete)        (LigmaStroke            *stroke,
                                          LigmaAnchor            *anchor);

  gboolean      (* point_is_movable)     (LigmaStroke            *stroke,
                                          LigmaAnchor            *predec,
                                          gdouble                position);
  void          (* point_move_relative)  (LigmaStroke            *stroke,
                                          LigmaAnchor            *predec,
                                          gdouble                position,
                                          const LigmaCoords      *deltacoord,
                                          LigmaAnchorFeatureType  feature);
  void          (* point_move_absolute)  (LigmaStroke            *stroke,
                                          LigmaAnchor            *predec,
                                          gdouble                position,
                                          const LigmaCoords      *coord,
                                          LigmaAnchorFeatureType  feature);

  void          (* close)                (LigmaStroke            *stroke);
  LigmaStroke  * (* open)                 (LigmaStroke            *stroke,
                                          LigmaAnchor            *end_anchor);
  gboolean      (* anchor_is_insertable) (LigmaStroke            *stroke,
                                          LigmaAnchor            *predec,
                                          gdouble                position);
  LigmaAnchor  * (* anchor_insert)        (LigmaStroke            *stroke,
                                          LigmaAnchor            *predec,
                                          gdouble                position);
  gboolean      (* is_extendable)        (LigmaStroke            *stroke,
                                          LigmaAnchor            *neighbor);
  LigmaAnchor  * (* extend)               (LigmaStroke            *stroke,
                                          const LigmaCoords      *coords,
                                          LigmaAnchor            *neighbor,
                                          LigmaVectorExtendMode   extend_mode);
  gboolean      (* connect_stroke)       (LigmaStroke            *stroke,
                                          LigmaAnchor            *anchor,
                                          LigmaStroke            *extension,
                                          LigmaAnchor            *neighbor);

  gboolean      (* is_empty)             (LigmaStroke            *stroke);
  gboolean      (* reverse)              (LigmaStroke            *stroke);
  gboolean      (* shift_start)          (LigmaStroke            *stroke,
                                          LigmaAnchor            *new_start);

  gdouble       (* get_length)           (LigmaStroke            *stroke,
                                          gdouble                precision);
  gdouble       (* get_distance)         (LigmaStroke            *stroke,
                                          const LigmaCoords      *coord);
  gboolean      (* get_point_at_dist)    (LigmaStroke            *stroke,
                                          gdouble                dist,
                                          gdouble                precision,
                                          LigmaCoords            *position,
                                          gdouble               *slope);

  GArray      * (* interpolate)          (LigmaStroke            *stroke,
                                          gdouble                precision,
                                          gboolean              *ret_closed);

  LigmaStroke  * (* duplicate)            (LigmaStroke            *stroke);

  LigmaBezierDesc * (* make_bezier)       (LigmaStroke            *stroke);

  void          (* translate)            (LigmaStroke            *stroke,
                                          gdouble                offset_x,
                                          gdouble                offset_y);
  void          (* scale)                (LigmaStroke            *stroke,
                                          gdouble                scale_x,
                                          gdouble                scale_y);
  void          (* rotate)               (LigmaStroke            *stroke,
                                          gdouble                center_x,
                                          gdouble                center_y,
                                          gdouble                angle);
  void          (* flip)                 (LigmaStroke             *stroke,
                                          LigmaOrientationType    flip_type,
                                          gdouble                axis);
  void          (* flip_free)            (LigmaStroke            *stroke,
                                          gdouble                x1,
                                          gdouble                y1,
                                          gdouble                x2,
                                          gdouble                y2);
  void          (* transform)            (LigmaStroke            *stroke,
                                          const LigmaMatrix3     *matrix,
                                          GQueue                *ret_strokes);

  GList       * (* get_draw_anchors)     (LigmaStroke            *stroke);
  GList       * (* get_draw_controls)    (LigmaStroke            *stroke);
  GArray      * (* get_draw_lines)       (LigmaStroke            *stroke);
  GArray      * (* control_points_get)   (LigmaStroke            *stroke,
                                          gboolean              *ret_closed);
};


GType        ligma_stroke_get_type             (void) G_GNUC_CONST;

void         ligma_stroke_set_id               (LigmaStroke            *stroke,
                                               gint                   id);
gint         ligma_stroke_get_id               (LigmaStroke            *stroke);


/* accessing / modifying the anchors */

GArray     * ligma_stroke_control_points_get   (LigmaStroke            *stroke,
                                               gboolean              *closed);

LigmaAnchor * ligma_stroke_anchor_get           (LigmaStroke            *stroke,
                                               const LigmaCoords      *coord);

gdouble      ligma_stroke_nearest_point_get    (LigmaStroke            *stroke,
                                               const LigmaCoords      *coord,
                                               gdouble                precision,
                                               LigmaCoords            *ret_point,
                                               LigmaAnchor           **ret_segment_start,
                                               LigmaAnchor           **ret_segment_end,
                                               gdouble               *ret_pos);
gdouble     ligma_stroke_nearest_tangent_get   (LigmaStroke            *stroke,
                                               const LigmaCoords      *coords1,
                                               const LigmaCoords      *coords2,
                                               gdouble                precision,
                                               LigmaCoords            *nearest,
                                               LigmaAnchor           **ret_segment_start,
                                               LigmaAnchor           **ret_segment_end,
                                               gdouble               *ret_pos);
gdouble  ligma_stroke_nearest_intersection_get (LigmaStroke            *stroke,
                                               const LigmaCoords      *coords1,
                                               const LigmaCoords      *direction,
                                               gdouble                precision,
                                               LigmaCoords            *nearest,
                                               LigmaAnchor           **ret_segment_start,
                                               LigmaAnchor           **ret_segment_end,
                                               gdouble               *ret_pos);


/* prev == NULL: "first" anchor */
LigmaAnchor * ligma_stroke_anchor_get_next      (LigmaStroke            *stroke,
                                               const LigmaAnchor      *prev);

void         ligma_stroke_anchor_select        (LigmaStroke            *stroke,
                                               LigmaAnchor            *anchor,
                                               gboolean               selected,
                                               gboolean               exclusive);

/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void         ligma_stroke_anchor_move_relative (LigmaStroke            *stroke,
                                               LigmaAnchor            *anchor,
                                               const LigmaCoords      *delta,
                                               LigmaAnchorFeatureType  feature);
void         ligma_stroke_anchor_move_absolute (LigmaStroke            *stroke,
                                               LigmaAnchor            *anchor,
                                               const LigmaCoords      *coord,
                                               LigmaAnchorFeatureType  feature);

gboolean     ligma_stroke_point_is_movable     (LigmaStroke            *stroke,
                                               LigmaAnchor            *predec,
                                               gdouble                position);
void         ligma_stroke_point_move_relative  (LigmaStroke            *stroke,
                                               LigmaAnchor            *predec,
                                               gdouble                position,
                                               const LigmaCoords      *deltacoord,
                                               LigmaAnchorFeatureType  feature);
void         ligma_stroke_point_move_absolute  (LigmaStroke            *stroke,
                                               LigmaAnchor            *predec,
                                               gdouble                position,
                                               const LigmaCoords      *coord,
                                               LigmaAnchorFeatureType  feature);

void         ligma_stroke_close                (LigmaStroke            *stroke);

void         ligma_stroke_anchor_convert       (LigmaStroke            *stroke,
                                               LigmaAnchor            *anchor,
                                               LigmaAnchorFeatureType  feature);

void         ligma_stroke_anchor_delete        (LigmaStroke            *stroke,
                                               LigmaAnchor            *anchor);

LigmaStroke * ligma_stroke_open                 (LigmaStroke            *stroke,
                                               LigmaAnchor            *end_anchor);
gboolean     ligma_stroke_anchor_is_insertable (LigmaStroke            *stroke,
                                               LigmaAnchor            *predec,
                                               gdouble                position);
LigmaAnchor * ligma_stroke_anchor_insert        (LigmaStroke            *stroke,
                                               LigmaAnchor            *predec,
                                               gdouble                position);

gboolean     ligma_stroke_is_extendable        (LigmaStroke            *stroke,
                                               LigmaAnchor            *neighbor);

LigmaAnchor * ligma_stroke_extend               (LigmaStroke            *stroke,
                                               const LigmaCoords      *coords,
                                               LigmaAnchor            *neighbor,
                                               LigmaVectorExtendMode   extend_mode);

gboolean     ligma_stroke_connect_stroke       (LigmaStroke            *stroke,
                                               LigmaAnchor            *anchor,
                                               LigmaStroke            *extension,
                                               LigmaAnchor            *neighbor);

gboolean     ligma_stroke_is_empty             (LigmaStroke            *stroke);

gboolean     ligma_stroke_reverse              (LigmaStroke            *stroke);
gboolean     ligma_stroke_shift_start          (LigmaStroke            *stroke,
                                               LigmaAnchor            *new_start);

/* accessing the shape of the curve */

gdouble      ligma_stroke_get_length           (LigmaStroke            *stroke,
                                               gdouble                precision);
gdouble      ligma_stroke_get_distance         (LigmaStroke            *stroke,
                                               const LigmaCoords      *coord);

gboolean     ligma_stroke_get_point_at_dist    (LigmaStroke            *stroke,
                                               gdouble                dist,
                                               gdouble                precision,
                                               LigmaCoords            *position,
                                               gdouble               *slope);

/* returns an array of valid coordinates */
GArray     * ligma_stroke_interpolate          (LigmaStroke            *stroke,
                                               const gdouble          precision,
                                               gboolean              *closed);

LigmaStroke * ligma_stroke_duplicate            (LigmaStroke            *stroke);

/* creates a bezier approximation. */
LigmaBezierDesc * ligma_stroke_make_bezier      (LigmaStroke            *stroke);

void         ligma_stroke_translate            (LigmaStroke            *stroke,
                                               gdouble                offset_x,
                                               gdouble                offset_y);
void         ligma_stroke_scale                (LigmaStroke            *stroke,
                                               gdouble                scale_x,
                                               gdouble                scale_y);
void         ligma_stroke_rotate               (LigmaStroke            *stroke,
                                               gdouble                center_x,
                                               gdouble                center_y,
                                               gdouble                angle);
void         ligma_stroke_flip                 (LigmaStroke             *stroke,
                                               LigmaOrientationType    flip_type,
                                               gdouble                axis);
void         ligma_stroke_flip_free            (LigmaStroke            *stroke,
                                               gdouble                x1,
                                               gdouble                y1,
                                               gdouble                x2,
                                               gdouble                y2);
void         ligma_stroke_transform            (LigmaStroke            *stroke,
                                               const LigmaMatrix3     *matrix,
                                               GQueue                *ret_strokes);


GList      * ligma_stroke_get_draw_anchors     (LigmaStroke            *stroke);
GList      * ligma_stroke_get_draw_controls    (LigmaStroke            *stroke);
GArray     * ligma_stroke_get_draw_lines       (LigmaStroke            *stroke);

#endif /* __LIGMA_STROKE_H__ */

