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


#define GIMP_TYPE_STROKE            (gimp_stroke_get_type ())
#define GIMP_STROKE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STROKE, GimpStroke))
#define GIMP_STROKE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STROKE, GimpStrokeClass))
#define GIMP_IS_STROKE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STROKE))
#define GIMP_IS_STROKE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_STROKE))
#define GIMP_STROKE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_STROKE, GimpStrokeClass))


struct _GimpStroke
{
  GObject     parent_instance;
           
  GList      *anchors;

  GimpStroke *next;

  GimpAnchor *temp_anchor;

  /* Stuff missing? */
};


struct _GimpStrokeClass
{
  GObjectClass  parent_class;

  void          (* changed)              (GimpStroke        *stroke);
                                                            
  void          (* removed)              (GimpStroke        *stroke);
                                                            
  GimpAnchor  * (* anchor_get)           (const GimpStroke  *stroke,
                                          const GimpCoords  *coord);
              
  GimpAnchor  * (* anchor_get_next)      (const GimpStroke  *stroke,
                                          const GimpAnchor  *prev);
              
  void          (* anchor_move_relative) (GimpStroke        *stroke,
                                          GimpAnchor        *anchor,
                                          const GimpCoords  *deltacoord,
                                          const gint         type);
              
  void          (* anchor_move_absolute) (GimpStroke        *stroke,
                                          GimpAnchor        *anchor,
					  const GimpCoords  *coord,
                                          const gint         type);
              
  void          (* anchor_delete)        (GimpStroke        *stroke,
                                          GimpAnchor        *anchor);
              
              
  gdouble       (* get_length)           (const GimpStroke  *stroke);
                                                           
  gdouble       (* get_distance)         (const GimpStroke  *stroke,
                                          const GimpCoords  *coord);
                                                          
  GimpCoords  * (* interpolate)          (const GimpStroke  *stroke,
                                          const gdouble      precision,
                                          gint              *ret_numcoords,
                                          gboolean          *ret_closed);
                                                          
                                                          
  GimpAnchor  * (* temp_anchor_get)      (const GimpStroke  *stroke);
                                                          
  GimpAnchor  * (* temp_anchor_set)      (GimpStroke        *stroke,
                                          const GimpCoords  *coord);
                                                          
  gboolean      (* temp_anchor_fix)      (GimpStroke        *stroke);

  GimpStroke  * (* make_bezier)          (const GimpStroke  *stroke);

};


/*  stroke utility functions  */

GType           gimp_stroke_get_type           (void) G_GNUC_CONST;


/* accessing / modifying the anchors */

GimpAnchor    * gimp_stroke_anchor_get         (const GimpStroke  *stroke,
                                                 const GimpCoords   *coord);
                                                                    
/* prev == NULL: "first" anchor */                                  
GimpAnchor    * gimp_stroke_anchor_get_next    (const GimpStroke  *stroke,
                                                 const GimpAnchor   *prev);
                                                                    
                                                                    
/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void            gimp_stroke_anchor_move_relative (GimpStroke        *stroke,
                                                   GimpAnchor       *anchor,
                                                   const GimpCoords *deltacoord,
                                                   const gint        type);

void            gimp_stroke_anchor_move_absolute (GimpStroke        *stroke,
                                                   GimpAnchor       *anchor,
                                                   const GimpCoords *coord,
                                                   const gint        type);

void            gimp_stroke_anchor_delete      (GimpStroke          *stroke,
                                                GimpAnchor          *anchor);


/* accessing the shape of the curve */

gdouble         gimp_stroke_get_length         (const GimpStroke    *stroke);
                                                                    
gdouble         gimp_stroke_get_distance       (const GimpStroke    *stroke,
                                                const GimpCoords    *coord);
                                                                    
/* returns the number of valid coordinates */                       
GimpCoords    * gimp_stroke_interpolate        (const GimpStroke *stroke,
                                                gdouble           precision,
                                                gint             *ret_numcoords,
                                                gboolean         *ret_closed);


/* Allow a singular temorary anchor (marking the "working point")? */

GimpAnchor    * gimp_stroke_temp_anchor_get    (const GimpStroke    *stroke);
                                                                    
GimpAnchor    * gimp_stroke_temp_anchor_set    (GimpStroke          *stroke,
                                                const GimpCoords    *coord);
                                                                    
gboolean        gimp_stroke_temp_anchor_fix    (GimpStroke          *stroke);


/* usually overloaded */

/* creates a bezier approximation. */

GimpStroke   * gimp_stroke_make_bezier        (const GimpStroke     *stroke);


#endif /* __GIMP_STROKE_H__ */
