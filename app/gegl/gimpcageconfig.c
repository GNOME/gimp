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
static void   gimp_cage_config_compute_edges_normal   (GimpCageConfig *gcc);

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
      printf("cgx: %.0f    cgy: %.0f    cvdx: %.0f    cvdy: %.0f  sf: %.2f  normx: %.2f  normy: %.2f %s\n",
             gcc->cage_points[i].src_point.x + ((gcc->cage_mode==GIMP_CAGE_MODE_CAGE_CHANGE)?gcc->displacement_x:0),
             gcc->cage_points[i].src_point.y + ((gcc->cage_mode==GIMP_CAGE_MODE_CAGE_CHANGE)?gcc->displacement_y:0),
             gcc->cage_points[i].dest_point.x + ((gcc->cage_mode==GIMP_CAGE_MODE_DEFORM)?gcc->displacement_x:0),
             gcc->cage_points[i].dest_point.y + ((gcc->cage_mode==GIMP_CAGE_MODE_DEFORM)?gcc->displacement_y:0),
             gcc->cage_points[i].edge_scaling_factor,
             gcc->cage_points[i].edge_normal.x,
             gcc->cage_points[i].edge_normal.y,
             ((gcc->cage_points[i].selected) ? "S" : "NS"));
    }
  printf("bounding box: x: %d  y: %d  width: %d  height: %d\n", bounding_box.x, bounding_box.y, bounding_box.width, bounding_box.height);
  printf("disp x: %f  disp y: %f\n", gcc->displacement_x, gcc->displacement_y);
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

  gcc->cage_points[gcc->n_cage_vertices].src_point.x = x + DELTA;
  gcc->cage_points[gcc->n_cage_vertices].src_point.y = y + DELTA;

  gcc->cage_points[gcc->n_cage_vertices].dest_point.x = x + DELTA;
  gcc->cage_points[gcc->n_cage_vertices].dest_point.y = y + DELTA;

  gcc->n_cage_vertices++;

  gimp_cage_config_compute_scaling_factor (gcc);
  gimp_cage_config_compute_edges_normal (gcc);
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
  gimp_cage_config_compute_edges_normal (gcc);
}

/**
 * gimp_cage_config_get_point_coordinate:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, GIMP_CAGE_MODE_CAGE_CHANGE or GIMP_CAGE_MODE_DEFORM
 * @point_number: the index of the point to return
 *
 * Returns: the real position of the given point, as a GimpVector2
 */
GimpVector2
gimp_cage_config_get_point_coordinate  (GimpCageConfig  *gcc,
                                        GimpCageMode     mode,
                                        gint             point_number)
{
  GimpVector2 point = {0.0, 0.0};

  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), point);
  g_return_val_if_fail (point_number < gcc->n_cage_vertices, point);
  g_return_val_if_fail (point_number >= 0, point);

  if (gcc->cage_points[point_number].selected)
    {
      if (mode == GIMP_CAGE_MODE_CAGE_CHANGE)
        {
          point.x = gcc->cage_points[point_number].src_point.x + gcc->displacement_x;
          point.y = gcc->cage_points[point_number].src_point.y + gcc->displacement_y;
        }
      else
        {
          point.x = gcc->cage_points[point_number].dest_point.x + gcc->displacement_x;
          point.y = gcc->cage_points[point_number].dest_point.y + gcc->displacement_y;
        }
    }
  else
    {
      if (mode == GIMP_CAGE_MODE_CAGE_CHANGE)
        {
          point.x = gcc->cage_points[point_number].src_point.x;
          point.y = gcc->cage_points[point_number].src_point.y;
        }
      else
        {
          point.x = gcc->cage_points[point_number].dest_point.x;
          point.y = gcc->cage_points[point_number].dest_point.y;
        }
    }

  return point;
}

/**
 * gimp_cage_config_add_displacement:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, GIMP_CAGE_MODE_CAGE_CHANGE or GIMP_CAGE_MODE_DEFORM
 * @point_number: the point of the cage to move
 * @x: x displacement value
 * @y: y displacement value
 *
 * Add a displacement for all slected point of the cage.
 * This displacement need to be commited to become effective.
 */
void
gimp_cage_config_add_displacement (GimpCageConfig  *gcc,
                                   GimpCageMode     mode,
                                   gdouble          x,
                                   gdouble          y)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  gcc->cage_mode = mode;
  gcc->displacement_x = x;
  gcc->displacement_y = y;

  #ifdef DEBUG_CAGE
    print_cage (gcc);
  #endif
}

/**
 * gimp_cage_config_commit_displacement:
 * @gcc: the cage config
 *
 * Apply the displacement to the cage
 */
void
gimp_cage_config_commit_displacement  (GimpCageConfig *gcc)
{
  gint  i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->n_cage_vertices; i++)
    {
      if (gcc->cage_points[i].selected)
        {
          if (gcc->cage_mode == GIMP_CAGE_MODE_CAGE_CHANGE)
            {
              gcc->cage_points[i].src_point.x += gcc->displacement_x;
              gcc->cage_points[i].src_point.y += gcc->displacement_y;
              gcc->cage_points[i].dest_point.x += gcc->displacement_x;
              gcc->cage_points[i].dest_point.y += gcc->displacement_y;
            }
          else
            {
              gcc->cage_points[i].dest_point.x += gcc->displacement_x;
              gcc->cage_points[i].dest_point.y += gcc->displacement_y;
            }
        }
    }
  gimp_cage_config_compute_scaling_factor (gcc);
  gimp_cage_config_compute_edges_normal (gcc);
  gimp_cage_config_reset_displacement (gcc);
}

/**
 * gimp_cage_config_reset_displacement:
 * @gcc: the cage config
 *
 * Set the displacement to zero.
 */
void
gimp_cage_config_reset_displacement (GimpCageConfig  *gcc)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  gcc->displacement_x = 0.0;
  gcc->displacement_y = 0.0;
}

/**
 * gimp_cage_config_get_bounding_box:
 * @gcc: the cage config
 *
 * Compute the bounding box of the source cage
 *
 * Returns: the bounding box of the source cage, as a GeglRectangle
 */
GeglRectangle
gimp_cage_config_get_bounding_box (GimpCageConfig  *gcc)
{
  GeglRectangle bounding_box = { 0, };
  gint          i;

  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), bounding_box);
  g_return_val_if_fail (gcc->n_cage_vertices >= 0, bounding_box);

  if (gcc->cage_points[0].selected)
    {
      bounding_box.x = gcc->cage_points[0].src_point.x + gcc->displacement_x;
      bounding_box.y = gcc->cage_points[0].src_point.y + gcc->displacement_y;
    }
  else
    {
      bounding_box.x = gcc->cage_points[0].src_point.x;
      bounding_box.y = gcc->cage_points[0].src_point.y;
    }

  bounding_box.height = 0;
  bounding_box.width = 0;

  for (i = 1; i < gcc->n_cage_vertices; i++)
    {
      gdouble x,y;

      if (gcc->cage_points[i].selected)
        {
          x = gcc->cage_points[i].src_point.x + gcc->displacement_x;
          y = gcc->cage_points[i].src_point.y + gcc->displacement_y;
        }
      else
        {
          x = gcc->cage_points[i].src_point.x;
          y = gcc->cage_points[i].src_point.y;
        }

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
  gint          i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->n_cage_vertices / 2; i++)
    {
      temp = gcc->cage_points[i];
      gcc->cage_points[i] = gcc->cage_points[gcc->n_cage_vertices - i - 1];
      gcc->cage_points[gcc->n_cage_vertices - i - 1] = temp;
    }

  gimp_cage_config_compute_scaling_factor (gcc);
  gimp_cage_config_compute_edges_normal (gcc);
}

/**
 * gimp_cage_config_reverse_cage_if_needed:
 * @gcc: the cage config
 *
 * Since the cage need to be defined counter-clockwise to have the
 * topological inside in the actual 'physical' inside of the cage,
 * this function compute if the cage is clockwise or not, and reverse
 * the cage if needed.
 *
 * This function does not take into account an eventual displacement
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
 * gimp_cage_config_compute_scaling_factor:
 * @gcc: the cage config
 *
 * Update Green Coordinate scaling factor for the destination cage.
 * This function does not take into account an eventual displacement.
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
                        &gcc->cage_points[i].src_point,
                        &gcc->cage_points[(i + 1) % gcc->n_cage_vertices].src_point);
      length = gimp_vector2_length (&edge);

      gimp_vector2_sub (&edge,
                        &gcc->cage_points[i].dest_point,
                        &gcc->cage_points[(i + 1) % gcc->n_cage_vertices].dest_point);
      length_d = gimp_vector2_length (&edge);

      gcc->cage_points[i].edge_scaling_factor = length_d / length;
    }
}

/**
 * gimp_cage_config_compute_edges_normal:
 * @gcc: the cage config
 *
 * Update edges normal for the destination cage.
 * This function does not take into account an eventual displacement.
 */
static void
gimp_cage_config_compute_edges_normal (GimpCageConfig *gcc)
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
 * This function does not take into account an eventual displacement.
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

/**
 * gimp_cage_config_select_point:
 * @gcc: the cage config
 * @point_number: the index of the point to select
 *
 * Select the given point of the cage, and deselect the others.
 */
void
gimp_cage_config_select_point (GimpCageConfig  *gcc,
                               gint             point_number)
{
  gint  i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->n_cage_vertices);
  g_return_if_fail (point_number >= 0);

  for (i = 0; i < gcc->n_cage_vertices; i++)
    {
      if (i == point_number)
        {
          gcc->cage_points[i].selected = TRUE;
        }
      else
        {
          gcc->cage_points[i].selected = FALSE;
        }
    }
}

/**
 * gimp_cage_config_select_area:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, GIMP_CAGE_MODE_CAGE_CHANGE or GIMP_CAGE_MODE_DEFORM
 * @area: the area to select
 *
 * Select cage's point inside the given area and deselect others
 */
void
gimp_cage_config_select_area  (GimpCageConfig  *gcc,
                               GimpCageMode     mode,
                               GeglRectangle    area)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  gimp_cage_config_deselect_points (gcc);
  gimp_cage_config_select_add_area (gcc, mode, area);
}

/**
 * gimp_cage_config_select_add_area:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, GIMP_CAGE_MODE_CAGE_CHANGE or GIMP_CAGE_MODE_DEFORM
 * @area: the area to select
 *
 * Select cage's point inside the given area. Already selected point stay selected.
 */
void
gimp_cage_config_select_add_area  (GimpCageConfig  *gcc,
                                   GimpCageMode     mode,
                                   GeglRectangle    area)
{
  gint  i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->n_cage_vertices; i++)
    {
      if (mode == GIMP_CAGE_MODE_CAGE_CHANGE)
        {
          if (gcc->cage_points[i].src_point.x >= area.x &&
              gcc->cage_points[i].src_point.x <= area.x + area.width &&
              gcc->cage_points[i].src_point.y >= area.y &&
              gcc->cage_points[i].src_point.y <= area.y + area.height)
            {
              gcc->cage_points[i].selected = TRUE;
            }
        }
      else
        {
          if (gcc->cage_points[i].dest_point.x >= area.x &&
              gcc->cage_points[i].dest_point.x <= area.x + area.width &&
              gcc->cage_points[i].dest_point.y >= area.y &&
              gcc->cage_points[i].dest_point.y <= area.y + area.height)
            {
              gcc->cage_points[i].selected = TRUE;
            }
        }
    }
}

/**
 * gimp_cage_config_toggle_point_selection:
 * @gcc: the cage config
 * @point_number: the index of the point to toggle selection
 *
 * Toggle the selection of the given cage point
 */
void
gimp_cage_config_toggle_point_selection (GimpCageConfig  *gcc,
                                         gint             point_number)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->n_cage_vertices);
  g_return_if_fail (point_number >= 0);

  gcc->cage_points[point_number].selected = ! gcc->cage_points[point_number].selected;
}

/**
 * gimp_cage_deselect_points:
 * @gcc: the cage config
 *
 * Deselect all cage points.
 */
void
gimp_cage_config_deselect_points (GimpCageConfig  *gcc)
{
  gint  i;

  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->n_cage_vertices; i++)
    {
      gcc->cage_points[i].selected = FALSE;
    }
}