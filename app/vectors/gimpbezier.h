/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * 
 * gimpbezier.h
 * Copyright (C) 2001 Simon Budig  <simon@gimp.org>
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

#ifndef __GIMP_BEZIER_H__
#define __GIMP_BEZIER_H__

/* Temporary implementation with straight lines. */

#include "config.h"

#include "glib-object.h"

#include "vectors-types.h"

#include "gimpvectors.h"

#define GIMP_TYPE_BEZIER            (gimp_bezier_get_type ())
#define GIMP_BEZIER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BEZIER, GimpBezier))
#define GIMP_BEZIER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BEZIER, GimpBezierClass))
#define GIMP_IS_BEZIER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BEZIER))
#define GIMP_IS_BEZIER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BEZIER))
#define GIMP_BEZIER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BEZIER, GimpBezierClass))


/* Bezier Data Structure */

struct _GimpBezier
{
  GimpVectors	parent_instance;

  GList	      * anchors;
  GList	      * strokes;
};


struct _GimpBezierClass
{
  GimpVectorsClass  parent_class;
};


GType           gimp_bezier_get_type           (void) G_GNUC_CONST;

/* accessing / modifying the anchors */

GimpAnchor    * gimp_bezier_anchor_get		(const GimpVectors *vectors,
						 const GimpCoords  *coord);

/* prev == NULL: "first" anchor */
GimpAnchor    * gimp_bezier_anchor_get_next	(const GimpVectors *vectors,
						 const GimpAnchor  *prev);

GimpAnchor    * gimp_bezier_anchor_set		(const GimpVectors *vectors,
						 const GimpCoords  *coord,
                                                 const gboolean     new_stroke);

void		gimp_bezier_anchor_move_relative (GimpVectors	   *vectors,
						  GimpAnchor	   *anchor,
                                                  const GimpCoords *deltacoord,
                                                  const gint        type);

void		gimp_bezier_anchor_move_absolute (GimpVectors	   *vectors,
						  GimpAnchor	   *anchor,
                                                  const GimpCoords *coord,
                                                  const gint        type);

void		gimp_bezier_anchor_delete	(GimpVectors	   *vectors,
						 GimpAnchor	   *anchor);


/* accessing the shape of the curve */

gdouble		gimp_bezier_get_length		(const GimpVectors *vectors,
						 const GimpAnchor  *start);

/* returns the number of valid coordinates */
gint		gimp_bezier_interpolate		(const GimpVectors *vectors,
						 const GimpStroke  *start,
						 const gdouble      precision,
						 const gint         max_points,
						 GimpCoords 	   *ret_coords);

/* Allow a singular temorary anchor (marking the "working point")? */

GimpAnchor    * gimp_bezier_temp_anchor_get	(const GimpVectors  *vectors);

GimpAnchor    * gimp_bezier_temp_anchor_set	(GimpVectors        *vectors,
                                                 const GimpCoords   *coord);

gboolean	gimp_bezier_temp_anchor_fix    (GimpVectors	   *vectors);

void		gimp_bezier_temp_anchor_delete (GimpVectors	   *vectors);

#endif /* __GIMP_BEZIER_H__ */
