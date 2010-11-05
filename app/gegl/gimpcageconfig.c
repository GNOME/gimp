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

  self->cage_vertices   = g_new0 (GimpVector2, self->max_cage_vertices);
  self->cage_vertices_d = g_new0 (GimpVector2, self->max_cage_vertices);
  self->scaling_factor  = g_malloc0 (self->max_cage_vertices * sizeof (gdouble));
  self->normal_d        = g_new0 (GimpVector2, self->max_cage_vertices);
}

static void
gimp_cage_config_finalize (GObject *object)
{
  GimpCageConfig  *gcc = GIMP_CAGE_CONFIG (object);

  g_free (gcc->cage_vertices);
  g_free (gcc->cage_vertices_d);
  g_free (gcc->scaling_factor);
  g_free (gcc->normal_d);

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

      gcc->cage_vertices = g_renew (GimpVector2,
                                    gcc->cage_vertices,
                                    gcc->max_cage_vertices);

      gcc->cage_vertices_d = g_renew (GimpVector2,
                                      gcc->cage_vertices_d,
                                      gcc->max_cage_vertices);

      gcc->scaling_factor = g_realloc (gcc->scaling_factor,
                                       gcc->max_cage_vertices * sizeof (gdouble));

      gcc->normal_d = g_renew (GimpVector2,
                               gcc->normal_d,
                               gcc->max_cage_vertices);
    }

  gcc->cage_vertices[gcc->n_cage_vertices].x = x + DELTA - gcc->offset_x;
  gcc->cage_vertices[gcc->n_cage_vertices].y = y + DELTA - gcc->offset_y;

  gcc->cage_vertices_d[gcc->n_cage_vertices].x = x + DELTA - gcc->offset_x;
  gcc->cage_vertices_d[gcc->n_cage_vertices].y = y + DELTA - gcc->offset_y;

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
      gcc->cage_vertices[point_number].x = x + DELTA - gcc->offset_x;
      gcc->cage_vertices[point_number].y = y + DELTA - gcc->offset_y;
      gcc->cage_vertices_d[point_number].x = x + DELTA - gcc->offset_x;
      gcc->cage_vertices_d[point_number].y = y + DELTA - gcc->offset_y;
    }
  else
    {
      gcc->cage_vertices_d[point_number].x = x + DELTA - gcc->offset_x;
      gcc->cage_vertices_d[point_number].y = y + DELTA - gcc->offset_y;
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

  bounding_box.x = gcc->cage_vertices[0].x;
  bounding_box.y = gcc->cage_vertices[0].y;
  bounding_box.height = 0;
  bounding_box.width = 0;

  for (i = 1; i < gcc->n_cage_vertices; i++)
    {
      gdouble x,y;

      x = gcc->cage_vertices[i].x;
      y = gcc->cage_vertices[i].y;

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
  GimpVector2 temp;
  gint        i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->n_cage_vertices / 2; i++)
    {
      temp = gcc->cage_vertices[i];
      gcc->cage_vertices[i] = gcc->cage_vertices[gcc->n_cage_vertices - i - 1];
      gcc->cage_vertices[gcc->n_cage_vertices - i - 1] = temp;

      temp = gcc->cage_vertices_d[i];
      gcc->cage_vertices_d[i] = gcc->cage_vertices_d[gcc->n_cage_vertices - i - 1];
      gcc->cage_vertices_d[gcc->n_cage_vertices - i - 1] = temp;
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

      P1 = gcc->cage_vertices[i];
      P2 = gcc->cage_vertices[(i+1) % gcc->n_cage_vertices];
      P3 = gcc->cage_vertices[(i+2) % gcc->n_cage_vertices];

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
 * gimp_cage_config_point_inside:
 * @gcc: the cage config
 * @x: x coordinate of the point to test
 * @y: y coordinate of the point to test
 *
 * Check if the given point is inside the cage. This test is done in
 * the regard of the topological inside of the cage.
 *
 * Returns: TRUE if the point is inside, FALSE if not.
 */
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
                        &gcc->cage_vertices[i],
                        &gcc->cage_vertices[(i + 1) % gcc->n_cage_vertices]);
      length = gimp_vector2_length (&edge);

      gimp_vector2_sub (&edge,
                        &gcc->cage_vertices_d[i],
                        &gcc->cage_vertices_d[(i + 1) % gcc->n_cage_vertices]);
      length_d = gimp_vector2_length (&edge);

      gcc->scaling_factor[i] = length_d / length;
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
                        &gcc->cage_vertices_d[(i + 1) % gcc->n_cage_vertices],
                        &gcc->cage_vertices_d[i]);

      gcc->normal_d[i] = gimp_vector2_normal (&normal);
    }
}

gboolean
gimp_cage_config_point_inside (GimpCageConfig *gcc,
                               gfloat          x,
                               gfloat          y)
{
  GimpVector2 *cv;
  gboolean     inside = FALSE;
  gint         i, j;

  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), FALSE);

  cv = gcc->cage_vertices;

  for (i = 0, j = gcc->n_cage_vertices - 1;
       i < gcc->n_cage_vertices;
       j = i++)
    {
      if ((((cv[i].y <= y) && (y < cv[j].y))
           || ((cv[j].y <= y) && (y < cv[i].y)))
          && (x < (cv[j].x - cv[i].x) * (y - cv[i].y) / (cv[j].y - cv[i].y) + cv[i].x))
        {
          inside = !inside;
        }
    }

  return inside;
}
