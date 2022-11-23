/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmavectors.h
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

#ifndef __LIGMA_VECTORS_H__
#define __LIGMA_VECTORS_H__

#include "core/ligmaitem.h"

#define LIGMA_TYPE_VECTORS            (ligma_vectors_get_type ())
#define LIGMA_VECTORS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VECTORS, LigmaVectors))
#define LIGMA_VECTORS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VECTORS, LigmaVectorsClass))
#define LIGMA_IS_VECTORS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_VECTORS))
#define LIGMA_IS_VECTORS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VECTORS))
#define LIGMA_VECTORS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VECTORS, LigmaVectorsClass))


typedef struct _LigmaVectorsClass  LigmaVectorsClass;

struct _LigmaVectors
{
  LigmaItem        parent_instance;

  GQueue         *strokes;        /* Queue of LigmaStrokes         */
  GHashTable     *stroke_to_list; /* Map from LigmaStroke to strokes listnode */
  gint            last_stroke_id;

  gint            freeze_count;
  gdouble         precision;

  LigmaBezierDesc *bezier_desc;    /* Cached bezier representation */

  gboolean        bounds_valid;   /* Cached bounding box          */
  gboolean        bounds_empty;
  gdouble         bounds_x1;
  gdouble         bounds_y1;
  gdouble         bounds_x2;
  gdouble         bounds_y2;
};

struct _LigmaVectorsClass
{
  LigmaItemClass  parent_class;

  /*  signals  */
  void          (* freeze)            (LigmaVectors       *vectors);
  void          (* thaw)              (LigmaVectors       *vectors);

  /*  virtual functions  */
  void          (* stroke_add)        (LigmaVectors       *vectors,
                                       LigmaStroke        *stroke);
  void          (* stroke_remove)     (LigmaVectors       *vectors,
                                       LigmaStroke        *stroke);
  LigmaStroke  * (* stroke_get)        (LigmaVectors       *vectors,
                                       const LigmaCoords  *coord);
  LigmaStroke  * (* stroke_get_next)   (LigmaVectors       *vectors,
                                       LigmaStroke        *prev);
  gdouble       (* stroke_get_length) (LigmaVectors       *vectors,
                                       LigmaStroke        *stroke);
  LigmaAnchor  * (* anchor_get)        (LigmaVectors       *vectors,
                                       const LigmaCoords  *coord,
                                       LigmaStroke       **ret_stroke);
  void          (* anchor_delete)     (LigmaVectors       *vectors,
                                       LigmaAnchor        *anchor);
  gdouble       (* get_length)        (LigmaVectors       *vectors,
                                       const LigmaAnchor  *start);
  gdouble       (* get_distance)      (LigmaVectors       *vectors,
                                       const LigmaCoords  *coord);
  gint          (* interpolate)       (LigmaVectors       *vectors,
                                       LigmaStroke        *stroke,
                                       gdouble            precision,
                                       gint               max_points,
                                       LigmaCoords        *ret_coords);
  LigmaBezierDesc * (* make_bezier)    (LigmaVectors       *vectors);
};


/*  vectors utility functions  */

GType           ligma_vectors_get_type           (void) G_GNUC_CONST;

LigmaVectors   * ligma_vectors_new                (LigmaImage         *image,
                                                 const gchar       *name);

LigmaVectors   * ligma_vectors_get_parent         (LigmaVectors       *vectors);

void            ligma_vectors_freeze             (LigmaVectors       *vectors);
void            ligma_vectors_thaw               (LigmaVectors       *vectors);

void            ligma_vectors_copy_strokes       (LigmaVectors       *src_vectors,
                                                 LigmaVectors       *dest_vectors);
void            ligma_vectors_add_strokes        (LigmaVectors       *src_vectors,
                                                 LigmaVectors       *dest_vectors);


/* accessing / modifying the anchors */

LigmaAnchor    * ligma_vectors_anchor_get         (LigmaVectors       *vectors,
                                                 const LigmaCoords  *coord,
                                                 LigmaStroke       **ret_stroke);

/* prev == NULL: "first" anchor */
LigmaAnchor    * ligma_vectors_anchor_get_next    (LigmaVectors        *vectors,
                                                 const LigmaAnchor   *prev);

/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void          ligma_vectors_anchor_move_relative (LigmaVectors        *vectors,
                                                 LigmaAnchor         *anchor,
                                                 const LigmaCoords   *deltacoord,
                                                 gint                type);
void          ligma_vectors_anchor_move_absolute (LigmaVectors        *vectors,
                                                 LigmaAnchor         *anchor,
                                                 const LigmaCoords   *coord,
                                                 gint                type);

void          ligma_vectors_anchor_delete        (LigmaVectors        *vectors,
                                                 LigmaAnchor         *anchor);

void          ligma_vectors_anchor_select        (LigmaVectors        *vectors,
                                                 LigmaStroke         *target_stroke,
                                                 LigmaAnchor         *anchor,
                                                 gboolean            selected,
                                                 gboolean            exclusive);


/* LigmaStroke is a connected component of a LigmaVectors object */

void            ligma_vectors_stroke_add         (LigmaVectors        *vectors,
                                                 LigmaStroke         *stroke);
void            ligma_vectors_stroke_remove      (LigmaVectors        *vectors,
                                                 LigmaStroke         *stroke);
gint            ligma_vectors_get_n_strokes      (LigmaVectors        *vectors);
LigmaStroke    * ligma_vectors_stroke_get         (LigmaVectors        *vectors,
                                                 const LigmaCoords   *coord);
LigmaStroke    * ligma_vectors_stroke_get_by_id   (LigmaVectors        *vectors,
                                                 gint                id);

/* prev == NULL: "first" stroke */
LigmaStroke    * ligma_vectors_stroke_get_next    (LigmaVectors        *vectors,
                                                 LigmaStroke         *prev);
gdouble         ligma_vectors_stroke_get_length  (LigmaVectors        *vectors,
                                                 LigmaStroke         *stroke);

/* accessing the shape of the curve */

gdouble         ligma_vectors_get_length         (LigmaVectors        *vectors,
                                                 const LigmaAnchor   *start);
gdouble         ligma_vectors_get_distance       (LigmaVectors        *vectors,
                                                 const LigmaCoords   *coord);

/* returns the number of valid coordinates */

gint            ligma_vectors_interpolate        (LigmaVectors        *vectors,
                                                 LigmaStroke         *stroke,
                                                 gdouble             precision,
                                                 gint                max_points,
                                                 LigmaCoords         *ret_coords);

/* usually overloaded */

/* returns a bezier representation */
const LigmaBezierDesc * ligma_vectors_get_bezier  (LigmaVectors        *vectors);


#endif /* __LIGMA_VECTORS_H__ */
