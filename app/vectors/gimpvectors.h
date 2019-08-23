/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectors.h
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#ifndef __GIMP_VECTORS_H__
#define __GIMP_VECTORS_H__

#include "core/gimpitem.h"

#define GIMP_TYPE_VECTORS            (gimp_vectors_get_type ())
#define GIMP_VECTORS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTORS, GimpVectors))
#define GIMP_VECTORS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VECTORS, GimpVectorsClass))
#define GIMP_IS_VECTORS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTORS))
#define GIMP_IS_VECTORS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VECTORS))
#define GIMP_VECTORS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VECTORS, GimpVectorsClass))


typedef struct _GimpVectorsClass  GimpVectorsClass;

struct _GimpVectors
{
  GimpItem        parent_instance;

  GQueue         *strokes;        /* Queue of GimpStrokes         */
  GHashTable     *stroke_to_list; /* Map from GimpStroke to strokes listnode */
  gint            last_stroke_id;

  gint            freeze_count;
  gdouble         precision;

  GimpBezierDesc *bezier_desc;    /* Cached bezier representation */

  gboolean        bounds_valid;   /* Cached bounding box          */
  gboolean        bounds_empty;
  gdouble         bounds_x1;
  gdouble         bounds_y1;
  gdouble         bounds_x2;
  gdouble         bounds_y2;
};

struct _GimpVectorsClass
{
  GimpItemClass  parent_class;

  /*  signals  */
  void          (* freeze)            (GimpVectors       *vectors);
  void          (* thaw)              (GimpVectors       *vectors);

  /*  virtual functions  */
  void          (* stroke_add)        (GimpVectors       *vectors,
                                       GimpStroke        *stroke);
  void          (* stroke_remove)     (GimpVectors       *vectors,
                                       GimpStroke        *stroke);
  GimpStroke  * (* stroke_get)        (GimpVectors       *vectors,
                                       const GimpCoords  *coord);
  GimpStroke  * (* stroke_get_next)   (GimpVectors       *vectors,
                                       GimpStroke        *prev);
  gdouble       (* stroke_get_length) (GimpVectors       *vectors,
                                       GimpStroke        *stroke);
  GimpAnchor  * (* anchor_get)        (GimpVectors       *vectors,
                                       const GimpCoords  *coord,
                                       GimpStroke       **ret_stroke);
  void          (* anchor_delete)     (GimpVectors       *vectors,
                                       GimpAnchor        *anchor);
  gdouble       (* get_length)        (GimpVectors       *vectors,
                                       const GimpAnchor  *start);
  gdouble       (* get_distance)      (GimpVectors       *vectors,
                                       const GimpCoords  *coord);
  gint          (* interpolate)       (GimpVectors       *vectors,
                                       GimpStroke        *stroke,
                                       gdouble            precision,
                                       gint               max_points,
                                       GimpCoords        *ret_coords);
  GimpBezierDesc * (* make_bezier)    (GimpVectors       *vectors);
};


/*  vectors utility functions  */

GType           gimp_vectors_get_type           (void) G_GNUC_CONST;

GimpVectors   * gimp_vectors_new                (GimpImage         *image,
                                                 const gchar       *name);

GimpVectors   * gimp_vectors_get_parent         (GimpVectors       *vectors);

void            gimp_vectors_freeze             (GimpVectors       *vectors);
void            gimp_vectors_thaw               (GimpVectors       *vectors);

void            gimp_vectors_copy_strokes       (GimpVectors       *src_vectors,
                                                 GimpVectors       *dest_vectors);
void            gimp_vectors_add_strokes        (GimpVectors       *src_vectors,
                                                 GimpVectors       *dest_vectors);


/* accessing / modifying the anchors */

GimpAnchor    * gimp_vectors_anchor_get         (GimpVectors       *vectors,
                                                 const GimpCoords  *coord,
                                                 GimpStroke       **ret_stroke);

/* prev == NULL: "first" anchor */
GimpAnchor    * gimp_vectors_anchor_get_next    (GimpVectors        *vectors,
                                                 const GimpAnchor   *prev);

/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void          gimp_vectors_anchor_move_relative (GimpVectors        *vectors,
                                                 GimpAnchor         *anchor,
                                                 const GimpCoords   *deltacoord,
                                                 gint                type);
void          gimp_vectors_anchor_move_absolute (GimpVectors        *vectors,
                                                 GimpAnchor         *anchor,
                                                 const GimpCoords   *coord,
                                                 gint                type);

void          gimp_vectors_anchor_delete        (GimpVectors        *vectors,
                                                 GimpAnchor         *anchor);

void          gimp_vectors_anchor_select        (GimpVectors        *vectors,
                                                 GimpStroke         *target_stroke,
                                                 GimpAnchor         *anchor,
                                                 gboolean            selected,
                                                 gboolean            exclusive);


/* GimpStroke is a connected component of a GimpVectors object */

void            gimp_vectors_stroke_add         (GimpVectors        *vectors,
                                                 GimpStroke         *stroke);
void            gimp_vectors_stroke_remove      (GimpVectors        *vectors,
                                                 GimpStroke         *stroke);
gint            gimp_vectors_get_n_strokes      (GimpVectors        *vectors);
GimpStroke    * gimp_vectors_stroke_get         (GimpVectors        *vectors,
                                                 const GimpCoords   *coord);
GimpStroke    * gimp_vectors_stroke_get_by_id   (GimpVectors        *vectors,
                                                 gint                id);

/* prev == NULL: "first" stroke */
GimpStroke    * gimp_vectors_stroke_get_next    (GimpVectors        *vectors,
                                                 GimpStroke         *prev);
gdouble         gimp_vectors_stroke_get_length  (GimpVectors        *vectors,
                                                 GimpStroke         *stroke);

/* accessing the shape of the curve */

gdouble         gimp_vectors_get_length         (GimpVectors        *vectors,
                                                 const GimpAnchor   *start);
gdouble         gimp_vectors_get_distance       (GimpVectors        *vectors,
                                                 const GimpCoords   *coord);

/* returns the number of valid coordinates */

gint            gimp_vectors_interpolate        (GimpVectors        *vectors,
                                                 GimpStroke         *stroke,
                                                 gdouble             precision,
                                                 gint                max_points,
                                                 GimpCoords         *ret_coords);

/* usually overloaded */

/* returns a bezier representation */
const GimpBezierDesc * gimp_vectors_get_bezier  (GimpVectors        *vectors);


#endif /* __GIMP_VECTORS_H__ */
