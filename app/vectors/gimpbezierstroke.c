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

#define INPUT_RESOLUTION 256
#define DETAIL 0.25


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
                                           GArray               **ret_coords);

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
  anchor->position.xtilt = 0.5;
  anchor->position.ytilt = 0.5;
  anchor->position.wheel = 0.5;

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

      /* FIXME */
      if (((GimpAnchor *) listneighbor->data)->type == 0)
        anchor->type = 1;
      else
        anchor->type = 0;

      g_printerr ("Extending at %f, %f, type %d\n", coords->x, coords->y, anchor->type);
    }
  else
    anchor = NULL;

  if (loose_end == 1)
    stroke->anchors = g_list_append (stroke->anchors, anchor);

  if (loose_end == -1)
    stroke->anchors = g_list_prepend (stroke->anchors, anchor);

  return anchor;
}


GArray *
gimp_bezier_stroke_interpolate (const GimpStroke  *stroke,
                                gdouble            precision,
                                gboolean          *ret_closed)
{
  GArray           *ret_coords;
  GimpAnchor       *anchor;
  GList            *anchorlist;
  GimpCoords        segmentcoords[4];
  gint              count;

  g_return_val_if_fail (GIMP_IS_BEZIER_STROKE (stroke), NULL);
  g_return_val_if_fail (ret_closed != NULL, NULL);
  
  ret_coords = g_array_new (FALSE, FALSE, sizeof (GimpCoords));

  count = 0;

  for (anchorlist = stroke->anchors; anchorlist;
       anchorlist = g_list_next (anchorlist))
    {
      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          gimp_bezier_coords_subdivide (segmentcoords, precision, &ret_coords);
          segmentcoords[0] = segmentcoords[3];
          count = 1;
        }
    }

  /* ret_coords = g_array_append_val (ret_coords, anchor->position); */

  *ret_closed    = FALSE;

  return ret_coords;
}


/* local helper functions for bezier subdivision */

/*   amul * a + bmul * b   */

static void
gimp_bezier_coords_mix (gdouble amul,
                        GimpCoords *a,
                        gdouble bmul,
                        GimpCoords *b,
                        GimpCoords *ret_val)
{
  if (b)
    {
      ret_val->x        = amul * a->x        + bmul * b->x ;
      ret_val->y        = amul * a->y        + bmul * b->y ;
      ret_val->pressure = amul * a->pressure + bmul * b->pressure ;
      ret_val->xtilt    = amul * a->xtilt    + bmul * b->xtilt ;
      ret_val->ytilt    = amul * a->ytilt    + bmul * b->ytilt ;
      ret_val->wheel    = amul * a->wheel    + bmul * b->wheel ;
    }
  else
    {
      ret_val->x        = amul * a->x;
      ret_val->y        = amul * a->y;
      ret_val->pressure = amul * a->pressure;
      ret_val->xtilt    = amul * a->xtilt;
      ret_val->ytilt    = amul * a->ytilt;
      ret_val->wheel    = amul * a->wheel;
    }
}

                        
/*    (a+b)/2   */

static void
gimp_bezier_coords_average (GimpCoords *a,
                            GimpCoords *b,
                            GimpCoords *ret_average)
{
  gimp_bezier_coords_mix (0.5, a, 0.5, b, ret_average);
}


/* a - b */

static void
gimp_bezier_coords_difference (GimpCoords *a,
                               GimpCoords *b,
                               GimpCoords *ret_difference)
{
  gimp_bezier_coords_mix (1.0, a, -1.0, b, ret_difference);
}


/* a * f = ret_product */

static void
gimp_bezier_coords_scale (gdouble     f,
                          GimpCoords *a,
                          GimpCoords *ret_multiply)
{
  gimp_bezier_coords_mix (f, a, 0.0, NULL, ret_multiply);
}


/* local helper for measuring the scalarproduct of two gimpcoords. */

static gdouble
gimp_bezier_coords_scalarprod (GimpCoords *a,
                               GimpCoords *b)
{
  return (a->x        * b->x        +
          a->y        * b->y        +
          a->pressure * b->pressure +
          a->xtilt    * b->xtilt    +
          a->ytilt    * b->ytilt    +
          a->wheel    * b->wheel   );
}


/*
 * The "lenght" of the gimpcoord.
 * Applies a metric that increases the weight on the
 * pressure/xtilt/ytilt/wheel to ensure proper interpolation
 */

static gdouble
gimp_bezier_coords_length2 (GimpCoords *a)
{
  GimpCoords upscaled_a;

  upscaled_a.x        = a->x;
  upscaled_a.y        = a->y;
  upscaled_a.pressure = a->pressure * INPUT_RESOLUTION;
  upscaled_a.xtilt    = a->xtilt    * INPUT_RESOLUTION;
  upscaled_a.ytilt    = a->ytilt    * INPUT_RESOLUTION;
  upscaled_a.wheel    = a->wheel    * INPUT_RESOLUTION;
  return (gimp_bezier_coords_scalarprod (&upscaled_a, &upscaled_a));
}


static gdouble
gimp_bezier_coords_length (GimpCoords *a)
{
  return (sqrt (gimp_bezier_coords_length2 (a)));
}


/*
 * a helper function that determines if a bezier segment is "straight
 * enough" to be approximated by a line.
 * 
 * Needs four GimpCoords in an array.
 */

static gboolean
gimp_bezier_coords_is_straight (GimpCoords *beziercoords)
{
  GimpCoords line, tan1, tan2, d1, d2;
  gdouble l2, s1, s2;

  gimp_bezier_coords_difference (&(beziercoords[3]),
                                 &(beziercoords[0]),
                                 &line);

  if (gimp_bezier_coords_length2 (&line) < DETAIL * DETAIL)
    {
      gimp_bezier_coords_difference (&(beziercoords[1]),
                                     &(beziercoords[0]),
                                     &tan1);
      gimp_bezier_coords_difference (&(beziercoords[2]),
                                     &(beziercoords[3]),
                                     &tan2);
      if ((gimp_bezier_coords_length2 (&tan1) < DETAIL * DETAIL) &&
          (gimp_bezier_coords_length2 (&tan2) < DETAIL * DETAIL))
        {
          return 1;
        }
      else
        {
          /* Tangents are too big for the small baseline */
          /* g_printerr ("Zu grosse Tangenten bei zu kleiner Basislinie\n"); */
          return 0;
        }
    }
  else
    {
      gimp_bezier_coords_difference (&(beziercoords[1]),
                                     &(beziercoords[0]),
                                     &tan1);
      gimp_bezier_coords_difference (&(beziercoords[2]),
                                     &(beziercoords[0]),
                                     &tan2);

      l2 = gimp_bezier_coords_scalarprod (&line, &line);
      s1 = gimp_bezier_coords_scalarprod (&line, &tan1) / l2;
      s2 = gimp_bezier_coords_scalarprod (&line, &tan2) / l2;

      if (s1 < 0 || s1 > 1 || s2 < 0 || s2 > 1 || s2 < s1)
        {
          /* The tangents get projected outside the baseline */
          /* g_printerr ("Tangenten projezieren sich ausserhalb der Basisline\n"); */
          return 0;
        }

      gimp_bezier_coords_mix (1.0, &tan1, - s1, &line, &d1);
      gimp_bezier_coords_mix (1.0, &tan2, - s2, &line, &d2);

      if ((gimp_bezier_coords_length2 (&d1) > DETAIL * DETAIL) ||
          (gimp_bezier_coords_length2 (&d2) > DETAIL * DETAIL))
        {
          /* The control points are too far away from the baseline */
          /* g_printerr ("Zu grosser Abstand zur Basislinie\n"); */
          return 0;
        }

      return 1;
    }
}


static void
gimp_bezier_coords_subdivide2 (GimpCoords     *beziercoords,
                               const gdouble   precision,
                               GArray        **ret_coords,
                               gint            depth)
{
  /*
   * beziercoords has to contain four GimpCoords with the four control points
   * of the bezier segment. We subdivide it at the parameter 0.5.
   */

  GimpCoords subdivided[8];

  subdivided[0] = beziercoords[0];
  subdivided[6] = beziercoords[3];

  if (!depth) g_printerr ("Hit rekursion depth limit!\n");

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

  if (!depth ||
      gimp_bezier_coords_is_straight (&(subdivided[0]))) /* 1st half */
    {
      *ret_coords = g_array_append_vals (*ret_coords, &(subdivided[0]), 3);
    }
  else
    {
      gimp_bezier_coords_subdivide2 (&(subdivided[0]), precision,
                                     ret_coords, depth-1);
    }

  if (!depth ||
      gimp_bezier_coords_is_straight (&(subdivided[3]))) /* 2nd half */
    {
      *ret_coords = g_array_append_vals (*ret_coords, &(subdivided[3]), 3);
    }
  else
    {
      gimp_bezier_coords_subdivide2 (&(subdivided[3]), precision,
                                     ret_coords, depth-1);
    }
  
  /* g_printerr ("gimp_bezier_coords_subdivide end: %d entries\n", (*ret_coords)->len); */
}


static void
gimp_bezier_coords_subdivide (GimpCoords     *beziercoords,
                              const gdouble   precision,
                              GArray        **ret_coords)
{
  gimp_bezier_coords_subdivide2 (beziercoords, precision, ret_coords, 10);
}

