/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectors.h
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

#ifndef __GIMP_VECTORS_H__
#define __GIMP_VECTORS_H__

#include "core/gimpviewable.h"


#define GIMP_TYPE_VECTORS            (gimp_vectors_get_type ())
#define GIMP_VECTORS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTORS, GimpVectors))
#define GIMP_VECTORS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VECTORS, GimpVectorsClass))
#define GIMP_IS_VECTORS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTORS))
#define GIMP_IS_VECTORS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VECTORS))
#define GIMP_VECTORS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VECTORS, GimpVectorsClass))


struct _GimpVectors
{
  GimpViewable      parent_instance;

  gboolean          visible;            /* controls visibility            */
  gboolean          locked;             /* transformation locking         */

  GList           * strokes;            /* The vectors components         */

  /* Stuff missing */
};


struct _GimpVectorsClass
{
  GimpViewableClass  parent_class;

  void          (* changed)              (GimpVectors       *vectors);
              
  void          (* removed)              (GimpVectors       *vectors);
              

  void          (* stroke_add)           (GimpVectors       *vectors,
                                          const GimpStroke  *stroke);
              
  GimpStroke  * (* stroke_get)           (const GimpVectors *vectors,
		                          const GimpCoords  *coord);
              
  GimpStroke  * (* stroke_get_next)      (const GimpVectors *vectors,
                                          const GimpStroke  *prev);
              
  gdouble       (* stroke_get_length)    (const GimpVectors *vectors,
                                          const GimpStroke  *stroke);
              

  GimpAnchor  * (* anchor_get)           (const GimpVectors *vectors,
                                          const GimpCoords  *coord);
              
  void          (* anchor_move_relative) (GimpVectors       *vectors,
                                          GimpAnchor        *anchor,
                                          const GimpCoords  *deltacoord,
                                          const gint         type);
              
  void          (* anchor_move_absolute) (GimpVectors       *vectors,
                                          GimpAnchor        *anchor,
                                          const GimpCoords  *coord,
                                          const gint         type);
              
  void          (* anchor_delete)        (GimpVectors       *vectors,
                                          GimpAnchor        *anchor);
              
  gdouble       (* get_length)           (const GimpVectors *vectors,
                                          const GimpAnchor  *start);
              
  gdouble       (* get_distance)         (const GimpVectors *vectors,
                                          const GimpCoords  *coord);
              
  gint          (* interpolate)          (const GimpVectors *vectors,
                                          const GimpStroke  *stroke,
                                          const gdouble      precision,
                                          const gint         max_points,
                                          GimpCoords        *ret_coords);
              
              
  GimpAnchor  * (* temp_anchor_get)      (const GimpVectors *vectors);
              
  GimpAnchor  * (* temp_anchor_set)      (GimpVectors       *vectors,
                                          const GimpCoords  *coord);
              
  gboolean      (* temp_anchor_fix)      (GimpVectors       *vectors);
  GimpVectors * (* make_bezier)          (const GimpVectors *vectors);

};


/*  vectors utility functions  */

GType           gimp_vectors_get_type           (void) G_GNUC_CONST;


/* accessing / modifying the anchors */

GimpAnchor    * gimp_vectors_anchor_get         (const GimpVectors  *vectors,
                                                 const GimpCoords   *coord);
                                                                    
/* prev == NULL: "first" anchor */                                  
GimpAnchor    * gimp_vectors_anchor_get_next    (const GimpVectors  *vectors,
                                                 const GimpAnchor   *prev);
                                                                    
/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void            gimp_vectors_anchor_move_relative (GimpVectors      *vectors,
                                                   GimpAnchor       *anchor,
                                                   const GimpCoords *deltacoord,
                                                   const gint        type);

void            gimp_vectors_anchor_move_absolute (GimpVectors      *vectors,
                                                   GimpAnchor       *anchor,
                                                   const GimpCoords *coord,
                                                   const gint        type);

void            gimp_vectors_anchor_delete      (GimpVectors        *vectors,
                                                 GimpAnchor         *anchor);

/* GimpStroke is a connected component of a GimpVectors object */

void            gimp_vectors_stroke_add         (GimpVectors       *vectors,
                                                 GimpStroke        *stroke);

GimpStroke    * gimp_vectors_stroke_get         (const GimpVectors  *vectors,
                                                 const GimpCoords   *coord);
                                                                    
/* prev == NULL: "first" stroke */                                  
GimpStroke    * gimp_vectors_stroke_get_next    (const GimpVectors  *vectors,
                                                 const GimpStroke   *prev);
                                                                    
gdouble         gimp_vectors_stroke_get_length  (const GimpVectors  *vectors,
                                                 const GimpStroke   *stroke);

/* accessing the shape of the curve */

gdouble         gimp_vectors_get_length         (const GimpVectors  *vectors,
                                                 const GimpAnchor   *start);
                                                                    
gdouble         gimp_vectors_get_distance       (const GimpVectors  *vectors,
                                                 const GimpCoords   *coord);
                                                                    
/* returns the number of valid coordinates */                       
gint            gimp_vectors_interpolate        (const GimpVectors  *vectors,
                                                 const GimpStroke   *stroke,
                                                 const gdouble       precision,
                                                 const gint          max_points,
                                                 GimpCoords         *ret_coords);


/* Allow a singular temorary anchor (marking the "working point")? */

GimpAnchor    * gimp_vectors_temp_anchor_get    (const GimpVectors  *vectors);
                                                                    
GimpAnchor    * gimp_vectors_temp_anchor_set    (GimpVectors        *vectors,
                                                 const GimpCoords   *coord);
                                                                    
gboolean        gimp_vectors_temp_anchor_fix    (GimpVectors        *vectors);


/* usually overloaded */

/* creates a bezier approximation. */

GimpVectors   * gimp_vectors_make_bezier        (const GimpVectors  *vectors);


#endif /* __GIMP_VECTORS_H__ */
