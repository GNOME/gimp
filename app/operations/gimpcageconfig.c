/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmacageconfig.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"

#include "operations-types.h"

#include "ligmacageconfig.h"


/*#define DEBUG_CAGE */

/* This DELTA is aimed to not have handle on exact pixel during computation,
 * to avoid particular case. It shouldn't be so useful, but it's a double
 * safety. */
#define DELTA             0.010309278351


static void   ligma_cage_config_finalize               (GObject        *object);
static void   ligma_cage_config_get_property           (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void   ligma_cage_config_set_property           (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);

static void   ligma_cage_config_compute_scaling_factor (LigmaCageConfig *gcc);
static void   ligma_cage_config_compute_edges_normal   (LigmaCageConfig *gcc);


G_DEFINE_TYPE_WITH_CODE (LigmaCageConfig, ligma_cage_config,
                         LIGMA_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                NULL))

#define parent_class ligma_cage_config_parent_class

#ifdef DEBUG_CAGE
static void
print_cage (LigmaCageConfig *gcc)
{
  gint i;
  GeglRectangle bounding_box;
  LigmaCagePoint *point;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  bounding_box = ligma_cage_config_get_bounding_box (gcc);

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      point = &g_array_index (gcc->cage_points, LigmaCagePoint, i);
      g_printerr ("cgx: %.0f    cgy: %.0f    cvdx: %.0f    cvdy: %.0f  sf: %.2f  normx: %.2f  normy: %.2f %s\n",
                  point->src_point.x + ((gcc->cage_mode==LIGMA_CAGE_MODE_CAGE_CHANGE)?gcc->displacement_x:0),
                  point->src_point.y + ((gcc->cage_mode==LIGMA_CAGE_MODE_CAGE_CHANGE)?gcc->displacement_y:0),
                  point->dest_point.x + ((gcc->cage_mode==LIGMA_CAGE_MODE_DEFORM)?gcc->displacement_x:0),
                  point->dest_point.y + ((gcc->cage_mode==LIGMA_CAGE_MODE_DEFORM)?gcc->displacement_y:0),
                  point->edge_scaling_factor,
                  point->edge_normal.x,
                  point->edge_normal.y,
                  ((point->selected) ? "S" : "NS"));
    }

  g_printerr ("bounding box: x: %d  y: %d  width: %d  height: %d\n", bounding_box.x, bounding_box.y, bounding_box.width, bounding_box.height);
  g_printerr ("disp x: %f  disp y: %f\n", gcc->displacement_x, gcc->displacement_y);
  g_printerr ("done\n");
}
#endif

static void
ligma_cage_config_class_init (LigmaCageConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_cage_config_set_property;
  object_class->get_property = ligma_cage_config_get_property;

  object_class->finalize     = ligma_cage_config_finalize;
}

static void
ligma_cage_config_init (LigmaCageConfig *self)
{
  /*pre-allocation for 50 vertices for the cage.*/
  self->cage_points = g_array_sized_new (FALSE, FALSE, sizeof(LigmaCagePoint), 50);
}

static void
ligma_cage_config_finalize (GObject *object)
{
  LigmaCageConfig *gcc = LIGMA_CAGE_CONFIG (object);

  g_array_free (gcc->cage_points, TRUE);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_cage_config_get_property (GObject    *object,
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
ligma_cage_config_set_property (GObject      *object,
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
 * ligma_cage_config_get_n_points:
 * @gcc: the cage config
 *
 * Returns: the number of points of the cage
 */
guint
ligma_cage_config_get_n_points (LigmaCageConfig *gcc)
{
  return gcc->cage_points->len;
}

/**
 * ligma_cage_config_add_cage_point:
 * @gcc: the cage config
 * @x: x value of the new point
 * @y: y value of the new point
 *
 * Add a new point in the last index of the polygon of the cage.
 * Point is added in both source and destination cage
 */
void
ligma_cage_config_add_cage_point (LigmaCageConfig  *gcc,
                                 gdouble          x,
                                 gdouble          y)
{
  ligma_cage_config_insert_cage_point (gcc, gcc->cage_points->len, x, y);
}

/**
 * ligma_cage_config_insert_cage_point:
 * @gcc: the cage config
 * @point_number: index where the point will be inserted
 * @x: x value of the new point
 * @y: y value of the new point
 *
 * Insert a new point in the polygon of the cage at the given index.
 * Point is added in both source and destination cage
 */
void
ligma_cage_config_insert_cage_point (LigmaCageConfig  *gcc,
                                    gint             point_number,
                                    gdouble          x,
                                    gdouble          y)
{
  LigmaCagePoint point;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number <= gcc->cage_points->len);
  g_return_if_fail (point_number >= 0);

  point.src_point.x = x + DELTA;
  point.src_point.y = y + DELTA;

  point.dest_point.x = x + DELTA;
  point.dest_point.y = y + DELTA;

  g_array_insert_val (gcc->cage_points, point_number, point);

  ligma_cage_config_compute_scaling_factor (gcc);
  ligma_cage_config_compute_edges_normal (gcc);
}

/**
 * ligma_cage_config_remove_last_cage_point:
 * @gcc: the cage config
 *
 * Remove the last point of the cage, in both source and destination cage
 */
void
ligma_cage_config_remove_last_cage_point (LigmaCageConfig  *gcc)
{
  ligma_cage_config_remove_cage_point (gcc, gcc->cage_points->len - 1);
}

/**
 * ligma_cage_config_remove_cage_point:
 * @gcc: the cage config
 * @point_number: the index of the point to remove
 *
 * Remove the given point from the cage
 */
void
ligma_cage_config_remove_cage_point (LigmaCageConfig *gcc,
                                    gint            point_number)
{
  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->cage_points->len);
  g_return_if_fail (point_number >= 0);

  if (gcc->cage_points->len > 0)
    g_array_remove_index (gcc->cage_points, gcc->cage_points->len - 1);

  ligma_cage_config_compute_scaling_factor (gcc);
  ligma_cage_config_compute_edges_normal (gcc);
}

/**
 * ligma_cage_config_remove_selected_points:
 * @gcc: the cage config
 *
 * Remove all the selected points from the cage
 */
void
ligma_cage_config_remove_selected_points (LigmaCageConfig  *gcc)
{
  gint           i;
  LigmaCagePoint *point;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      point = &g_array_index (gcc->cage_points, LigmaCagePoint, i);

      if (point->selected)
        {
          g_array_remove_index (gcc->cage_points, i);
          i--;
        }
    }

  ligma_cage_config_compute_scaling_factor (gcc);
  ligma_cage_config_compute_edges_normal (gcc);
}

/**
 * ligma_cage_config_get_point_coordinate:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, LIGMA_CAGE_MODE_CAGE_CHANGE or LIGMA_CAGE_MODE_DEFORM
 * @point_number: the index of the point to return
 *
 * Returns: the real position of the given point, as a LigmaVector2
 */
LigmaVector2
ligma_cage_config_get_point_coordinate (LigmaCageConfig *gcc,
                                       LigmaCageMode    mode,
                                       gint            point_number)
{
  LigmaVector2     result = { 0.0, 0.0 };
  LigmaCagePoint  *point;

  g_return_val_if_fail (LIGMA_IS_CAGE_CONFIG (gcc), result);
  g_return_val_if_fail (point_number < gcc->cage_points->len, result);
  g_return_val_if_fail (point_number >= 0, result);

  point = &g_array_index (gcc->cage_points, LigmaCagePoint, point_number);

  if (point->selected)
    {
      if (mode == LIGMA_CAGE_MODE_CAGE_CHANGE)
        {
          result.x = point->src_point.x + gcc->displacement_x;
          result.y = point->src_point.y + gcc->displacement_y;
        }
      else
        {
          result.x = point->dest_point.x + gcc->displacement_x;
          result.y = point->dest_point.y + gcc->displacement_y;
        }
    }
  else
    {
      if (mode == LIGMA_CAGE_MODE_CAGE_CHANGE)
        {
          result.x = point->src_point.x;
          result.y = point->src_point.y;
        }
      else
        {
          result.x = point->dest_point.x;
          result.y = point->dest_point.y;
        }
    }

  return result;
}

/**
 * ligma_cage_config_add_displacement:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, LIGMA_CAGE_MODE_CAGE_CHANGE or LIGMA_CAGE_MODE_DEFORM
 * @point_number: the point of the cage to move
 * @x: x displacement value
 * @y: y displacement value
 *
 * Add a displacement for all selected points of the cage.
 * This displacement need to be committed to become effective.
 */
void
ligma_cage_config_add_displacement (LigmaCageConfig *gcc,
                                   LigmaCageMode    mode,
                                   gdouble         x,
                                   gdouble         y)
{
  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  gcc->cage_mode = mode;
  gcc->displacement_x = x;
  gcc->displacement_y = y;

  #ifdef DEBUG_CAGE
    print_cage (gcc);
  #endif
}

/**
 * ligma_cage_config_commit_displacement:
 * @gcc: the cage config
 *
 * Apply the displacement to the cage
 */
void
ligma_cage_config_commit_displacement (LigmaCageConfig *gcc)
{
  gint  i;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      LigmaCagePoint *point;
      point = &g_array_index (gcc->cage_points, LigmaCagePoint, i);

      if (point->selected)
        {
          if (gcc->cage_mode == LIGMA_CAGE_MODE_CAGE_CHANGE)
            {
              point->src_point.x += gcc->displacement_x;
              point->src_point.y += gcc->displacement_y;
              point->dest_point.x += gcc->displacement_x;
              point->dest_point.y += gcc->displacement_y;
            }
          else
            {
              point->dest_point.x += gcc->displacement_x;
              point->dest_point.y += gcc->displacement_y;
            }
        }
    }

  ligma_cage_config_compute_scaling_factor (gcc);
  ligma_cage_config_compute_edges_normal (gcc);
  ligma_cage_config_reset_displacement (gcc);
}

/**
 * ligma_cage_config_reset_displacement:
 * @gcc: the cage config
 *
 * Set the displacement to zero.
 */
void
ligma_cage_config_reset_displacement (LigmaCageConfig *gcc)
{
  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  gcc->displacement_x = 0.0;
  gcc->displacement_y = 0.0;
}

/**
 * ligma_cage_config_get_bounding_box:
 * @gcc: the cage config
 *
 * Compute the bounding box of the source cage
 *
 * Returns: the bounding box of the source cage, as a GeglRectangle
 */
GeglRectangle
ligma_cage_config_get_bounding_box (LigmaCageConfig *gcc)
{
  GeglRectangle  bounding_box = { 0, 0, 0, 0};
  gint           i;
  LigmaCagePoint *point;

  g_return_val_if_fail (LIGMA_IS_CAGE_CONFIG (gcc), bounding_box);

  if (gcc->cage_points->len == 0)
    return bounding_box;

  point = &g_array_index (gcc->cage_points, LigmaCagePoint, 0);

  if (point->selected)
    {
      bounding_box.x = point->src_point.x + gcc->displacement_x;
      bounding_box.y = point->src_point.y + gcc->displacement_y;
    }
  else
    {
      bounding_box.x = point->src_point.x;
      bounding_box.y = point->src_point.y;
    }

  for (i = 1; i < gcc->cage_points->len; i++)
    {
      gdouble x,y;
      point = &g_array_index (gcc->cage_points, LigmaCagePoint, i);

      if (point->selected)
        {
          x = point->src_point.x + gcc->displacement_x;
          y = point->src_point.y + gcc->displacement_y;
        }
      else
        {
          x = point->src_point.x;
          y = point->src_point.y;
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
 * ligma_cage_config_reverse_cage:
 * @gcc: the cage config
 *
 * When using non-simple cage (like a cage in 8), user may want to
 * manually inverse inside and outside of the cage. This function
 * reverse the cage
 */
void
ligma_cage_config_reverse_cage (LigmaCageConfig *gcc)
{
  LigmaCagePoint temp;
  gint          i;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len / 2; i++)
    {
      temp = g_array_index (gcc->cage_points, LigmaCagePoint, i);

      g_array_index (gcc->cage_points, LigmaCagePoint, i) =
        g_array_index (gcc->cage_points, LigmaCagePoint, gcc->cage_points->len - i - 1);

      g_array_index (gcc->cage_points, LigmaCagePoint, gcc->cage_points->len - i - 1) = temp;
    }

  ligma_cage_config_compute_scaling_factor (gcc);
  ligma_cage_config_compute_edges_normal (gcc);
}

/**
 * ligma_cage_config_reverse_cage_if_needed:
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
ligma_cage_config_reverse_cage_if_needed (LigmaCageConfig *gcc)
{
  gint    i;
  gdouble sum;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  sum = 0.0;

  /* this is a bit crappy, but should works most of the case */
  /* we do the sum of the projection of each point to the previous
     segment, and see the final sign */
  for (i = 0; i < gcc->cage_points->len ; i++)
    {
      LigmaVector2 P1, P2, P3;
      gdouble     z;

      P1 = (g_array_index (gcc->cage_points, LigmaCagePoint, i)).src_point;
      P2 = (g_array_index (gcc->cage_points, LigmaCagePoint, (i+1) % gcc->cage_points->len)).src_point;
      P3 = (g_array_index (gcc->cage_points, LigmaCagePoint, (i+2) % gcc->cage_points->len)).src_point;

      z = P1.x * (P2.y - P3.y) + P2.x * (P3.y - P1.y) + P3.x * (P1.y - P2.y);

      sum += z;
    }

  /* sum > 0 mean a cage defined counter-clockwise, so we reverse it */
  if (sum > 0)
    {
      ligma_cage_config_reverse_cage (gcc);
    }
}

/**
 * ligma_cage_config_compute_scaling_factor:
 * @gcc: the cage config
 *
 * Update Green Coordinate scaling factor for the destination cage.
 * This function does not take into account an eventual displacement.
 */
static void
ligma_cage_config_compute_scaling_factor (LigmaCageConfig *gcc)
{
  LigmaVector2    edge;
  gdouble        length, length_d;
  gint           i;
  LigmaCagePoint *current, *last;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));
  if (gcc->cage_points->len < 2)
    return;

  last = &g_array_index (gcc->cage_points, LigmaCagePoint, 0);

  for (i = 1; i <= gcc->cage_points->len; i++)
    {
      current = &g_array_index (gcc->cage_points, LigmaCagePoint, i % gcc->cage_points->len);

      ligma_vector2_sub (&edge,
                        &(last->src_point),
                        &(current->src_point));
      length = ligma_vector2_length (&edge);

      ligma_vector2_sub (&edge,
                        &(last->dest_point),
                        &(current->dest_point));
      length_d = ligma_vector2_length (&edge);

      last->edge_scaling_factor = length_d / length;
      last = current;
    }
}

/**
 * ligma_cage_config_compute_edges_normal:
 * @gcc: the cage config
 *
 * Update edges normal for the destination cage.
 * This function does not take into account an eventual displacement.
 */
static void
ligma_cage_config_compute_edges_normal (LigmaCageConfig *gcc)
{
  LigmaVector2    normal;
  gint           i;
  LigmaCagePoint *current, *last;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  last = &g_array_index (gcc->cage_points, LigmaCagePoint, 0);

  for (i = 1; i <= gcc->cage_points->len; i++)
    {
      current = &g_array_index (gcc->cage_points, LigmaCagePoint, i % gcc->cage_points->len);

      ligma_vector2_sub (&normal,
                        &(current->dest_point),
                        &(last->dest_point));

      last->edge_normal = ligma_vector2_normal (&normal);
      last = current;
    }
}

/**
 * ligma_cage_config_point_inside:
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
ligma_cage_config_point_inside (LigmaCageConfig *gcc,
                               gfloat          x,
                               gfloat          y)
{
  LigmaVector2   *last, *current;
  gboolean       inside = FALSE;
  gint           i;

  g_return_val_if_fail (LIGMA_IS_CAGE_CONFIG (gcc), FALSE);

  last = &((g_array_index (gcc->cage_points, LigmaCagePoint, gcc->cage_points->len - 1)).src_point);

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      current = &((g_array_index (gcc->cage_points, LigmaCagePoint, i)).src_point);

      if ((((current->y <= y) && (y < last->y))
           || ((last->y <= y) && (y < current->y)))
          && (x < (last->x - current->x) * (y - current->y) / (last->y - current->y) + current->x))
        {
          inside = !inside;
        }

      last = current;
    }

  return inside;
}

/**
 * ligma_cage_config_select_point:
 * @gcc: the cage config
 * @point_number: the index of the point to select
 *
 * Select the given point of the cage, and deselect the others.
 */
void
ligma_cage_config_select_point (LigmaCageConfig  *gcc,
                               gint             point_number)
{
  gint           i;
  LigmaCagePoint *point;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->cage_points->len);
  g_return_if_fail (point_number >= 0);

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      point = &g_array_index (gcc->cage_points, LigmaCagePoint, i);

      if (i == point_number)
        {
          point->selected = TRUE;
        }
      else
        {
          point->selected = FALSE;
        }
    }
}

/**
 * ligma_cage_config_select_area:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, LIGMA_CAGE_MODE_CAGE_CHANGE or LIGMA_CAGE_MODE_DEFORM
 * @area: the area to select
 *
 * Select cage's point inside the given area and deselect others
 */
void
ligma_cage_config_select_area  (LigmaCageConfig  *gcc,
                               LigmaCageMode     mode,
                               GeglRectangle    area)
{
  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  ligma_cage_config_deselect_points (gcc);
  ligma_cage_config_select_add_area (gcc, mode, area);
}

/**
 * ligma_cage_config_select_add_area:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, LIGMA_CAGE_MODE_CAGE_CHANGE or LIGMA_CAGE_MODE_DEFORM
 * @area: the area to select
 *
 * Select cage's point inside the given area. Already selected point stay selected.
 */
void
ligma_cage_config_select_add_area (LigmaCageConfig *gcc,
                                  LigmaCageMode    mode,
                                  GeglRectangle   area)
{
  gint           i;
  LigmaCagePoint *point;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      point = &g_array_index (gcc->cage_points, LigmaCagePoint, i);

      if (mode == LIGMA_CAGE_MODE_CAGE_CHANGE)
        {
          if (point->src_point.x >= area.x &&
              point->src_point.x <= area.x + area.width &&
              point->src_point.y >= area.y &&
              point->src_point.y <= area.y + area.height)
            {
              point->selected = TRUE;
            }
        }
      else
        {
          if (point->dest_point.x >= area.x &&
              point->dest_point.x <= area.x + area.width &&
              point->dest_point.y >= area.y &&
              point->dest_point.y <= area.y + area.height)
            {
              point->selected = TRUE;
            }
        }
    }
}

/**
 * ligma_cage_config_toggle_point_selection:
 * @gcc: the cage config
 * @point_number: the index of the point to toggle selection
 *
 * Toggle the selection of the given cage point
 */
void
ligma_cage_config_toggle_point_selection (LigmaCageConfig *gcc,
                                         gint            point_number)
{
  LigmaCagePoint *point;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->cage_points->len);
  g_return_if_fail (point_number >= 0);

  point = &g_array_index (gcc->cage_points, LigmaCagePoint, point_number);
  point->selected = ! point->selected;
}

/**
 * ligma_cage_deselect_points:
 * @gcc: the cage config
 *
 * Deselect all cage points.
 */
void
ligma_cage_config_deselect_points (LigmaCageConfig *gcc)
{
  gint  i;

  g_return_if_fail (LIGMA_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      (g_array_index (gcc->cage_points, LigmaCagePoint, i)).selected = FALSE;
    }
}

/**
 * ligma_cage_config_point_is_selected:
 * @gcc: the cage config
 * @point_number: the index of the point to test
 *
 * Returns: TRUE if the point is selected, FALSE otherwise.
 */
gboolean
ligma_cage_config_point_is_selected (LigmaCageConfig  *gcc,
                                    gint             point_number)
{
  LigmaCagePoint *point;

  g_return_val_if_fail (LIGMA_IS_CAGE_CONFIG (gcc), FALSE);
  g_return_val_if_fail (point_number < gcc->cage_points->len, FALSE);
  g_return_val_if_fail (point_number >= 0, FALSE);

  point = &(g_array_index (gcc->cage_points, LigmaCagePoint, point_number));

  return point->selected;
}
