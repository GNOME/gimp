/* GIMP - The GNU Image Manipulation Program
 *
 * gimpcageconfig.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpcageconfig.h"


/*#define DEBUG_CAGE */

#define N_ITEMS_PER_ALLOC 10
/* This DELTA is aimed to not have handle on exact pixel during computation,
 * to avoid particular case. It shouldn't be so usefull, but it's a double
 * safety. */
#define DELTA             0.010309278351


static void   gimp_cage_config_finalize               (GObject        *object);
static void   gimp_cage_config_get_property           (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void   gimp_cage_config_set_property           (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);

static void   gimp_cage_config_compute_scaling_factor (GimpCageConfig *gcc);
static void   gimp_cage_config_compute_edge_normal    (GimpCageConfig *gcc);


G_DEFINE_TYPE_WITH_CODE (GimpCageConfig, gimp_cage_config,
                         GIMP_TYPE_IMAGE_MAP_CONFIG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                NULL))

#define parent_class gimp_cage_config_parent_class

#ifdef DEBUG_CAGE
static void
print_cage (GimpCageConfig *gcc)
{
  gint i;
  GeglRectangle bounding_box;
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  bounding_box = gimp_cage_config_get_bounding_box (gcc);

  for (i = 0; i < gcc->n_cage_vertices; i++)
    {
      printf("cgx: %.0f    cgy: %.0f    cvdx: %.0f    cvdy: %.0f  sf: %.2f  normx: %.2f  normy: %.2f\n", gcc->cage_vertices[i].x, gcc->cage_vertices[i].y, gcc->cage_vertices_d[i].x,  gcc->cage_vertices_d[i].y, gcc->scaling_factor[i], gcc->normal_d[i].x, gcc->normal_d[i].y);
    }
  printf("bounding box: x: %d  y: %d  width: %d  height: %d\n", bounding_box.x, bounding_box.y, bounding_box.width, bounding_box.height);
  printf("offsets: (%d, %d)", gcc->offset_x, gcc->offset_y);
  printf("done\n");
}
#endif

static void
gimp_cage_config_class_init (GimpCageConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_cage_config_set_property;
  object_class->get_property = gimp_cage_config_get_property;

  object_class->finalize     = gimp_cage_config_finalize;
}

static void
gimp_cage_config_init (GimpCageConfig *self)
{
  self->n_cage_vertices   = 0;
  self->max_cage_vertices = 50; /*pre-allocation for 50 vertices for the cage.*/

  self->cage_points   = g_new0 (GimpCagePoint, self->max_cage_vertices);
}

static void
gimp_cage_config_finalize (GObject *object)
{
  GimpCageConfig  *gcc = GIMP_CAGE_CONFIG (object);

  g_free (gcc->cage_points);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_cage_config_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_cage_config_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_cage_config_add_cage_point:
 * @gcc: the cage config
 * @x: x value of the new point
 * @y: y value of the new point
 *
 * Add a new point in the polygon of the cage, and make allocation if needed.
 * Point is added in both source and destination cage
 */
void
gimp_cage_config_add_cage_point (GimpCageConfig  *gcc,
                                 gdouble          x,
                                 gdouble          y)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  /* reallocate memory if needed */
  if (gcc->n_cage_vertices >= gcc->max_cage_vertices)
    {
      gcc->max_cage_vertices += N_ITEMS_PER_ALLOC;

      gcc->cage_points = g_renew (GimpCagePoint,
                                  gcc->cage_points,
                                  gcc->max_cage_vertices);
    }

  gcc->cage_points[gcc->n_cage_vertices].src_point.x = x + DELTA - gcc->offset_x;
  gcc->cage_points[gcc->n_cage_vertices].src_point.y = y + DELTA - gcc->offset_y;

  gcc->cage_points[gcc->n_cage_vertices].dest_point.x = x + DELTA - gcc->offset_x;
  gcc->cage_points[gcc->n_cage_vertices].dest_point.y = y + DELTA - gcc->offset_y;

  gcc->n_cage_vertices++;

  gimp_cage_config_compute_scaling_factor (gcc);
  gimp_cage_config_compute_edge_normal (gcc);
}

/**
 * gimp_cage_config_remove_last_cage_point:
 * @gcc: the cage config
 *
 * Remove the last point of the cage, in both source and destination cage
 */
void
gimp_cage_config_remove_last_cage_point (GimpCageConfig  *gcc)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  if (gcc->n_cage_vertices >= 1)
    gcc->n_cage_vertices--;

  gimp_cage_config_compute_scaling_factor (gcc);
  gimp_cage_config_compute_edge_normal (gcc);
}

/**
 * gimp_cage_config_move_cage_point:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, GIMP_CAGE_MODE_CAGE_CHANGE or GIMP_CAGE_MODE_DEFORM
 * @point_number: the point of the cage to move
 * @x: new x value
 * @y: new y value
 *
 * Move a point of the source or destination cage, according to the
 * cage mode provided
 */
void
gimp_cage_config_move_cage_point (GimpCageConfig  *gcc,
                                  GimpCageMode     mode,
                                  gint             point_number,
                                  gdouble          x,
                                  gdouble          y)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->n_cage_vertices);
  g_return_if_fail (point_number >= 0);

  if (mode == GIMP_CAGE_MODE_CAGE_CHANGE)
    {
      gcc->cage_points[point_number].src_point.x = x + DELTA - gcc->offset_x;
      gcc->cage_points[point_number].src_point.y = y + DELTA - gcc->offset_y;
      gcc->cage_points[point_number].dest_point.x = x + DELTA - gcc->offset_x;
      gcc->cage_points[point_number].dest_point.y = y + DELTA - gcc->offset_y;
    }
  else
    {
      gcc->cage_points[point_number].dest_point.x = x + DELTA - gcc->offset_x;
      gcc->cage_points[point_number].dest_point.y = y + DELTA - gcc->offset_y;
    }

  gimp_cage_config_compute_scaling_factor (gcc);
  gimp_cage_config_compute_edge_normal (gcc);
}

/**
 * gimp_cage_config_get_bounding_box:
 * @gcc: the cage config
 *
 * Compute the bounding box of the destination cage
 *
 * Returns: the bounding box of the destination cage, as a GeglRectangle
 */
GeglRectangle
gimp_cage_config_get_bounding_box (GimpCageConfig  *gcc)
{
  GeglRectangle bounding_box = { 0, };
  gint          i;

  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), bounding_box);
  g_return_val_if_fail (gcc->n_cage_vertices >= 0, bounding_box);

  bounding_box.x = gcc->cage_points[0].src_point.x;
  bounding_box.y = gcc->cage_points[0].src_point.y;
  bounding_box.height = 0;
  bounding_box.width = 0;

  for (i = 1; i < gcc->n_cage_vertices; i++)
    {
      gdouble x,y;

      x = gcc->cage_points[i].src_point.x;
      y = gcc->cage_points[i].src_point.y;

      if (x < bounding_box.x)
        {
          bounding_box.width += bounding_box.x - x;
          bounding_box.x = x;
        }

      if (y < bounding_box.y)
        {
          bounding_box.height += bounding_box.y - y;
          bounding_box.y = y;
        }

      if (x > bounding_box.x + bounding_box.width)
        {
          bounding_box.width = x - bounding_box.x;
        }

      if (y > bounding_box.y + bounding_box.height)
        {
          bounding_box.height = y - bounding_box.y;
        }
    }

  return bounding_box;
}

/**
 * gimp_cage_config_reverse_cage:
 * @gcc: the cage config
 *
 * When using non-simple cage (like a cage in 8), user may want to
 * manually inverse inside and outside of the cage. This function
 * reverse the cage
 */
void
gimp_cage_config_reverse_cage (GimpCageConfig *gcc)
{
  GimpCagePoint temp;
  gint        i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->n_cage_vertices / 2; i++)
    {
      temp = gcc->cage_points[i];
      gcc->cage_points[i] = gcc->cage_points[gcc->n_cage_vertices - i - 1];
      gcc->cage_points[gcc->n_cage_vertices - i - 1] = temp;
    }

  gimp_cage_config_compute_scaling_factor (gcc);
  gimp_cage_config_compute_edge_normal (gcc);
}

/**
 * gimp_cage_config_reverse_cage_if_needed:
 * @gcc: the cage config
 *
 * Since the cage need to be defined counter-clockwise to have the
 * topological inside in the actual 'physical' inside of the cage,
 * this function compute if the cage is clockwise or not, and reverse
 * the cage if needed.
 */
void
gimp_cage_config_reverse_cage_if_needed (GimpCageConfig *gcc)
{
  gint    i;
  gdouble sum;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  sum = 0.0;

  /* this is a bit crappy, but should works most of the case */
  /* we do the sum of the projection of each point to the previous
     segment, and see the final sign */
  for (i = 0; i < gcc->n_cage_vertices ; i++)
    {
      GimpVector2 P1, P2, P3;
      gdouble     z;

      P1 = gcc->cage_points[i].src_point;
      P2 = gcc->cage_points[(i+1) % gcc->n_cage_vertices].src_point;
      P3 = gcc->cage_points[(i+2) % gcc->n_cage_vertices].src_point;

      z = P1.x * (P2.y - P3.y) + P2.x * (P3.y - P1.y) + P3.x * (P1.y - P2.y);

      sum += z;
    }

  /* sum > 0 mean a cage defined counter-clockwise, so we reverse it */
  if (sum > 0)
    {
      gimp_cage_config_reverse_cage (gcc);
    }
}

/**
 * gimp_cage_config_get_cage_points:
 * @gcc: the cage config
 *
 * Returns: a copy of the cage's point
 */
GimpCagePoint*
gimp_cage_config_get_cage_points (GimpCageConfig  *gcc)
{
  gint            i;
  GimpCagePoint  *points_copy;

  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), NULL);

  points_copy = g_new (GimpCagePoint, gcc->n_cage_vertices);

  for(i = 0; i < gcc->n_cage_vertices; i++)
    {
      points_copy[i] = gcc->cage_points[i];
    }

  return points_copy;
}

/**
 * gimp_cage_config_commit_cage_points:
 * @gcc: the cage config
 * @points: a GimpCagePoint array of point of the same length as the number of point in the cage
 *
 * This function update the cage's point with the array provided.
 */
void
gimp_cage_config_commit_cage_points (GimpCageConfig  *gcc,
                                     GimpCagePoint   *points)
{
  gint          i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for(i = 0; i < gcc->n_cage_vertices; i++)
    {
      gcc->cage_points[i] = points[i];
    }
}

static void
gimp_cage_config_compute_scaling_factor (GimpCageConfig *gcc)
{
  GimpVector2 edge;
  gdouble     length, length_d;
  gint        i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->n_cage_vertices; i++)
    {
      gimp_vector2_sub (&edge,
                        &gcc->cage_points[i].src_point,
                        &gcc->cage_points[(i + 1) % gcc->n_cage_vertices].src_point);
      length = gimp_vector2_length (&edge);

      gimp_vector2_sub (&edge,
                        &gcc->cage_points[i].dest_point,
                        &gcc->cage_points[(i + 1) % gcc->n_cage_vertices].dest_point);
      length_d = gimp_vector2_length (&edge);

      gcc->cage_points[i].edge_scaling_factor = length_d / length;
    }

#ifdef DEBUG_CAGE
  print_cage (gcc);
#endif
}

static void
gimp_cage_config_compute_edge_normal (GimpCageConfig *gcc)
{
  GimpVector2 normal;
  gint        i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->n_cage_vertices; i++)
    {
      gimp_vector2_sub (&normal,
                        &gcc->cage_points[(i + 1) % gcc->n_cage_vertices].dest_point,
                        &gcc->cage_points[i].dest_point);

      gcc->cage_points[i].edge_normal = gimp_vector2_normal (&normal);
    }
}

/**
 * gimp_cage_config_point_inside:
 * @gcc: the cage config
 * @x: x coordinate of the point to test
 * @y: y coordinate of the point to test
 *
 * Check if the given point is inside the cage. This test is done in
 * the regard of the topological inside of the source cage.
 *
 * Returns: TRUE if the point is inside, FALSE if not.
 */
gboolean
gimp_cage_config_point_inside (GimpCageConfig *gcc,
                               gfloat          x,
                               gfloat          y)
{
  GimpVector2 *cpi, *cpj;
  gboolean     inside = FALSE;
  gint         i, j;

  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), FALSE);

  for (i = 0, j = gcc->n_cage_vertices - 1;
       i < gcc->n_cage_vertices;
       j = i++)
    {
      cpi = &(gcc->cage_points[i].src_point);
      cpj = &(gcc->cage_points[j].src_point);

      if ((((cpi->y <= y) && (y < cpj->y))
           || ((cpj->y <= y) && (y < cpi->y)))
          && (x < (cpj->x - cpi->x) * (y - cpi->y) / (cpj->y - cpi->y) + cpi->x))
        {
          inside = !inside;
        }
    }

  return inside;
}
