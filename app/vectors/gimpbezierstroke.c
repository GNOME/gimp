/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstroke.c
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


#include "config.h"

#include "glib-object.h"

#include "vectors-types.h"

#include "gimpanchor.h"
#include "gimpbezierstroke.h"



/*  private variables  */


static GimpStrokeClass *parent_class = NULL;


static void gimp_bezier_stroke_class_init (GimpBezierStrokeClass *klass);
static void gimp_bezier_stroke_init       (GimpBezierStroke      *bezier_stroke);

static void gimp_bezier_stroke_finalize   (GObject               *object);

static void gimp_bezier_coords_average    (GimpCoords            *a,
                                           GimpCoords            *b,
                                           GimpCoords            *ret_average);

static void gimp_bezier_coords_subdivide  (GimpCoords            *beziercoords,
                                           const gdouble          precision,
                                           gint                  *ret_ncoords,
                                           GimpCoords            *ret_coords);

GType
gimp_bezier_stroke_get_type (void)
{
  static GType bezier_stroke_type = 0;

  if (! bezier_stroke_type)
    {
      static const GTypeInfo bezier_stroke_info =
      {
        sizeof (GimpBezierStrokeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_bezier_stroke_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpBezierStroke),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_bezier_stroke_init,
      };

      bezier_stroke_type = g_type_register_static (GIMP_TYPE_STROKE,
                                                   "GimpBezierStroke", 
                                                   &bezier_stroke_info, 0);
    }

  return bezier_stroke_type;
}

static void
gimp_bezier_stroke_class_init (GimpBezierStrokeClass *klass)
{
  GObjectClass      *object_class;
  GimpStrokeClass   *stroke_class;

  object_class = G_OBJECT_CLASS (klass);
  stroke_class = GIMP_STROKE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize             = gimp_bezier_stroke_finalize;

  stroke_class->interpolate          = gimp_bezier_stroke_interpolate;

}

static void
gimp_bezier_stroke_init (GimpBezierStroke *bezier_stroke)
{
  /* pass */
};

static void
gimp_bezier_stroke_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* Bezier specific functions */

GimpStroke *
gimp_bezier_stroke_new (const GimpCoords *start)
{
  GimpBezierStroke *bezier_stroke;
  GimpStroke       *stroke;
  GimpAnchor       *anchor;

  bezier_stroke = g_object_new (GIMP_TYPE_BEZIER_STROKE, NULL);

  stroke = GIMP_STROKE (bezier_stroke);

  anchor = g_new0 (GimpAnchor, 1);
  anchor->position.x = start->x;
  anchor->position.y = start->y;
  anchor->position.pressure = 1;

  g_printerr ("Adding at %f, %f\n", start->x, start->y);
  
  anchor->type = 0;    /* FIXME */

  stroke->anchors = g_list_append (stroke->anchors, anchor);
  return stroke;
}


GimpStroke *
gimp_bezier_stroke_new_from_coords (const GimpCoords *coords,
                                    const gint        ncoords)
{
  GimpBezierStroke *bezier_stroke;
  GimpStroke       *stroke = NULL;
  GimpAnchor       *last_anchor;

  gint              count;

  if (ncoords >= 1)
    {
      stroke = gimp_bezier_stroke_new (coords);
      bezier_stroke = GIMP_BEZIER_STROKE (stroke);
      last_anchor = (GimpAnchor *) (stroke->anchors->data);

      count = 1;
      while (count < ncoords)
        {
          last_anchor = gimp_bezier_stroke_extend (bezier_stroke,
                                                   &(coords[count]),
                                                   last_anchor);
        }

    }

  return stroke;
}


GimpAnchor *
gimp_bezier_stroke_extend (GimpBezierStroke *bezier_stroke,
                           const GimpCoords *coords,
                           GimpAnchor       *neighbor)
{
  GimpAnchor       *anchor;
  GimpStroke       *stroke;
  GList            *listneighbor;
  gint              loose_end;

  g_return_val_if_fail (GIMP_IS_BEZIER_STROKE (bezier_stroke), NULL);
  g_return_val_if_fail ((neighbor != NULL), NULL);

  stroke = GIMP_STROKE (bezier_stroke);

  listneighbor = g_list_last (stroke->anchors);

  loose_end = 0;

  if (listneighbor->data != neighbor)
    {
      listneighbor = g_list_first (stroke->anchors);
      if (listneighbor->data != neighbor)
        {
          listneighbor = NULL;
          loose_end = 0;
        }
      else
        {
          loose_end = -1;
        }
    }
  else
    {
      loose_end = 1;
    }

  if (loose_end)
    {
      anchor = g_new0 (GimpAnchor, 1);
      anchor->position.x = coords->x;
      anchor->position.y = coords->y;
      anchor->position.pressure = 1;
      anchor->position.xtilt = 0.5;
      anchor->position.ytilt = 0.5;
      anchor->position.wheel = 0.5;

      anchor->type = 0;    /* FIXME */

      g_printerr ("Extending at %f, %f\n", coords->x, coords->y);
    }
  else
    anchor = NULL;

  if (loose_end == 1)
    stroke->anchors = g_list_append (stroke->anchors, anchor);

  if (loose_end == -1)
    stroke->anchors = g_list_prepend (stroke->anchors, anchor);

  return anchor;
}


GimpCoords *
gimp_bezier_stroke_interpolate (const GimpStroke  *stroke,
                                gdouble            precision,
                                gint              *ret_numcoords,
                                gboolean          *ret_closed)
{
  gint              count, alloccount;
  gint              chunksize = 100;
  GimpCoords       *ret_coords;
  GimpAnchor       *anchor;
  GList            *anchorlist;

  g_return_val_if_fail (GIMP_IS_BEZIER_STROKE (stroke), NULL);
  g_return_val_if_fail (ret_numcoords != NULL, NULL);
  g_return_val_if_fail (ret_closed != NULL, NULL);
  
  count = 0;
  alloccount = 0;
  ret_coords = NULL;

  for (anchorlist = stroke->anchors; anchorlist;
       anchorlist = g_list_next (anchorlist))
    {
      if (count >= alloccount)
        {
          ret_coords = g_renew (GimpCoords, ret_coords, alloccount + chunksize);
          alloccount += chunksize;
        }
      
      anchor = anchorlist->data;
      ret_coords[count] = anchor->position;

      count++;
    }

  *ret_numcoords = count;
  *ret_closed    = FALSE;

  return ret_coords;
}


/* local helper functions for bezier subdivision */

static void
gimp_bezier_coords_average (GimpCoords *a,
                            GimpCoords *b,
                            GimpCoords *ret_average)
{
  ret_average->x        = (a->x        + b->x       ) / 2.0;
  ret_average->y        = (a->y        + b->y       ) / 2.0;
  ret_average->pressure = (a->pressure + b->pressure) / 2.0;
  ret_average->xtilt    = (a->xtilt    + b->xtilt   ) / 2.0;
  ret_average->ytilt    = (a->ytilt    + b->ytilt   ) / 2.0;
  ret_average->wheel    = (a->wheel    + b->wheel   ) / 2.0;
}


static void
gimp_bezier_coords_subdivide (GimpCoords    *beziercoords,
                              const gdouble  precision,
                              gint          *ret_ncoords,
                              GimpCoords    *ret_coords)
{
  /*
   * beziercoords has to contain four GimpCoords with the four control points
   * of the bezier segment. We subdivide it at the parameter 0.5.
   */

  GimpCoords subdivided[8];
  gint i, good_enough = 1;

  subdivided[0] = beziercoords[0];
  subdivided[6] = beziercoords[3];

  gimp_bezier_coords_average (&(beziercoords[0]), &(beziercoords[1]),
                              &(subdivided[1]));

  gimp_bezier_coords_average (&(beziercoords[1]), &(beziercoords[2]),
                              &(subdivided[7]));

  gimp_bezier_coords_average (&(beziercoords[2]), &(beziercoords[3]),
                              &(subdivided[5]));

  gimp_bezier_coords_average (&(subdivided[1]), &(subdivided[7]),
                              &(subdivided[2]));

  gimp_bezier_coords_average (&(subdivided[7]), &(subdivided[5]),
                              &(subdivided[4]));

  gimp_bezier_coords_average (&(subdivided[2]), &(subdivided[4]),
                              &(subdivided[3]));

  /*
   * We now have the coordinates of the two bezier segments in
   * subdivided [0-3] and subdivided [3-6]
   */

  /*
   * Here we need to check, if we have sufficiently subdivided, i.e.
   * if the stroke is sufficiently close to a straight line.
   */

  if ( good_enough /* stroke 1 */ )
    {
      for (i=0; i < 3; i++)
        {
          /* if necessary, allocate new memory for return coords */
          ret_coords[*ret_ncoords] = subdivided[i];
          (*ret_ncoords)++;
        }
    }
  else
    {
      gimp_bezier_coords_subdivide (&(subdivided[0]), precision,
                                    ret_ncoords, ret_coords);
    }

  if ( good_enough /* stroke 2 */ )
    {
      for (i=0; i < 3; i++)
        {
          /* if necessary, allocate new memory for return coords */
          ret_coords[*ret_ncoords] = subdivided[i+3];
          (*ret_ncoords)++;
        }
    }
  else
    {
      gimp_bezier_coords_subdivide (&(subdivided[3]), precision,
                                    ret_ncoords, ret_coords);
    }
  
  if ( /* last iteration */ 0)
    {
      /* if necessary, allocate new memory for return coords */
      ret_coords[*ret_ncoords] = subdivided[6];
      (*ret_ncoords)++;
    }
}
