/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectors.c
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


#include "config.h"

#include "glib-object.h"

#include "vectors-types.h"

#include "gimpvectors.h"



/*  private variables  */


static GimpVectorsClass *parent_class = NULL;


static void
gimp_vectors_finalize (GObject *object)
{
  /* blablabla */
}


static void
gimp_vectors_class_init (GimpVectorsClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize             = gimp_vectors_finalize;

  /* gimp_object_class->name_changed    = gimp_vectors_name_changed; */

  klass->changed                     = NULL;
  klass->removed                     = NULL;

  klass->anchor_get		     = NULL;
  klass->anchor_get_next	     = NULL;
  klass->anchor_set		     = NULL;
  klass->anchor_move_relative	     = NULL;
  klass->anchor_move_absolute	     = NULL;
  klass->anchor_delete		     = NULL;

  klass->stroke_get		     = NULL;
  klass->stroke_get_next	     = NULL;
  klass->stroke_get_length	     = NULL;

  klass->get_length		     = NULL;
  klass->get_distance		     = NULL;
  klass->interpolate		     = NULL;

  klass->temp_anchor_get	     = NULL;
  klass->temp_anchor_set	     = NULL;
  klass->temp_anchor_fix	     = NULL;

  klass->make_bezier		     = NULL;
}


GType
gimp_vectors_get_type (void)
{
  static GType vectors_type = 0;

  if (! vectors_type)
    {
      static const GTypeInfo vectors_info =
      {
        sizeof (GimpVectorsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_vectors_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpVectors),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) NULL,
      };

      vectors_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
                                            "GimpVectors", 
                                            &vectors_info, 0);
    }

  return vectors_type;
}


/* Calling the virtual functions */

GimpAnchor *
gimp_vectors_anchor_get (const GimpVectors *vectors,
			 const GimpCoords  *coord)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->anchor_get)
    return vectors_class->anchor_get (vectors, coord);
  else
    g_printerr ("gimp_vectors_anchor_get: default implementation\n");

  return NULL;
}


GimpAnchor *
gimp_vectors_anchor_get_next (const GimpVectors *vectors,
		              const GimpAnchor  *prev)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->anchor_get_next)
    return vectors_class->anchor_get_next (vectors, prev);
  else
    g_printerr ("gimp_vectors_anchor_get_next: default implementation\n");

  return NULL;
}


GimpAnchor *
gimp_vectors_anchor_set (const GimpVectors *vectors,
			 const GimpCoords  *coord,
                         const gboolean     new_stroke)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->anchor_set)
    return vectors_class->anchor_set (vectors, coord, new_stroke);
  else
    g_printerr ("gimp_vectors_anchor_set: default implementation\n");

  return NULL;
}


void
gimp_vectors_anchor_move_relative (GimpVectors       *vectors,
                                   GimpAnchor        *anchor,
				   const GimpCoords  *deltacoord,
                                   const gint         type)
{
  GimpVectorsClass *vectors_class;

  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->anchor_move_relative)
    vectors_class->anchor_move_relative (vectors, anchor, deltacoord, type);
  else
    g_printerr ("gimp_vectors_anchor_move_relative: default implementation\n");

  return;
}


void
gimp_vectors_anchor_move_absolute (GimpVectors       *vectors,
                                   GimpAnchor        *anchor,
				   const GimpCoords  *coord,
                                   const gint         type)
{
  GimpVectorsClass *vectors_class;

  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->anchor_move_absolute)
    vectors_class->anchor_move_absolute (vectors, anchor, coord, type);
  else
    g_printerr ("gimp_vectors_anchor_move_absolute: default implementation\n");

  return;
}


void
gimp_vectors_anchor_delete (GimpVectors     *vectors,
                            GimpAnchor      *anchor)
{
  GimpVectorsClass *vectors_class;

  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->anchor_delete)
    vectors_class->anchor_delete (vectors, anchor);
  else
    g_printerr ("gimp_vectors_anchor_delete: default implementation\n");

  return;
}


GimpStroke *
gimp_vectors_stroke_get (const GimpVectors *vectors,
			 const GimpCoords  *coord)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->stroke_get)
    return vectors_class->stroke_get (vectors, coord);
  else
    g_printerr ("gimp_vectors_stroke_get: default implementation\n");

  return NULL;
}


GimpStroke *
gimp_vectors_stroke_get_next (const GimpVectors *vectors,
                              const GimpStroke  *prev)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->stroke_get_next)
    return vectors_class->stroke_get_next (vectors, prev);
  else
    g_printerr ("gimp_vectors_stroke_get_next: default implementation\n");

  return NULL;
}


gdouble
gimp_vectors_stroke_get_length (const GimpVectors *vectors,
                                const GimpStroke  *prev)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->stroke_get_length)
    return vectors_class->stroke_get_length (vectors, prev);
  else
    g_printerr ("gimp_vectors_stroke_get_length: default implementation\n");

  return 0;
}


gdouble
gimp_vectors_get_length (const GimpVectors *vectors,
                         const GimpAnchor  *start)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->get_length)
    return vectors_class->get_length (vectors, start);
  else
    g_printerr ("gimp_vectors_get_length: default implementation\n");

  return 0;
}


gdouble
gimp_vectors_get_distance (const GimpVectors *vectors,
			   const GimpCoords  *coord)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->get_distance)
    return vectors_class->get_distance (vectors, coord);
  else
    g_printerr ("gimp_vectors_get_distance: default implementation\n");

  return 0;
}


gint
gimp_vectors_interpolate (const GimpVectors *vectors,
                          const GimpStroke  *stroke,
                          const gdouble      precision,
                          const gint         max_points,
                          GimpCoords        *ret_coords)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), 0);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->interpolate)
    return vectors_class->interpolate (vectors, stroke, precision, max_points, ret_coords);
  else
    g_printerr ("gimp_vectors_interpolate: default implementation\n");

  return 0;
}


GimpAnchor *
gimp_vectors_temp_anchor_get (const GimpVectors *vectors)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->temp_anchor_get)
    return vectors_class->temp_anchor_get (vectors);
  else
    g_printerr ("gimp_vectors_temp_anchor_get: default implementation\n");

  return NULL;
}


GimpAnchor *
gimp_vectors_temp_anchor_set (GimpVectors *vectors,
			      const GimpCoords  *coord)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->temp_anchor_set)
    return vectors_class->temp_anchor_set (vectors, coord);
  else
    g_printerr ("gimp_vectors_temp_anchor_set: default implementation\n");

  return NULL;
}


gboolean
gimp_vectors_temp_anchor_fix (GimpVectors *vectors)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->temp_anchor_fix)
    return vectors_class->temp_anchor_fix (vectors);
  else
    g_printerr ("gimp_vectors_temp_anchor_fix: default implementation\n");

  return FALSE;
}


GimpVectors *
gimp_vectors_make_bezier (const GimpVectors *vectors)
{
  GimpVectorsClass *vectors_class;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);

  vectors_class = GIMP_VECTORS_GET_CLASS (vectors);

  if (vectors_class->make_bezier)
    return vectors_class->make_bezier (vectors);
  else
    g_printerr ("gimp_vectors_make_bezier: default implementation\n");

  return NULL;
}




