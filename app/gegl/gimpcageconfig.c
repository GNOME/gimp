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
#include "libgimpmodule/gimpmodule.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"
#include "libgimpmath/gimpmathtypes.h"
#include "libgimpbase/gimpbaseenums.h"

#include "libgimpmath/gimpvector.h"
#include <math.h>

#include <stdio.h>

#include "gimpcageconfig.h"

G_DEFINE_TYPE_WITH_CODE (GimpCageConfig, gimp_cage_config,
                         GIMP_TYPE_IMAGE_MAP_CONFIG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                NULL))

#define parent_class gimp_cage_config_parent_class

#define N_ITEMS_PER_ALLOC       10

static void       gimp_cage_config_finalize                   (GObject *object);
static void       gimp_cage_config_get_property               (GObject    *object,
                                                               guint       property_id,
                                                               GValue     *value,
                                                               GParamSpec *pspec);
static void       gimp_cage_config_set_property               (GObject      *object,
                                                               guint         property_id,
                                                               const GValue *value,
                                                               GParamSpec   *pspec);
static void       gimp_cage_config_compute_scaling_factor     (GimpCageConfig *gcc);

/* FIXME: to debug only */
static void
print_cage (GimpCageConfig *gcc)
{
  gint i;
  GeglRectangle bounding_box;
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
      
  bounding_box = gimp_cage_config_get_bounding_box (gcc);
  
  for (i = 0; i < gcc->cage_vertice_number; i++)
  {
    printf("cgx: %.0f    cgy: %.0f    cvdx: %.0f    cvdy: %.0f  sf: %.2f\n", gcc->cage_vertices[i].x, gcc->cage_vertices[i].y, gcc->cage_vertices_d[i].x,  gcc->cage_vertices_d[i].y, gcc->scaling_factor[i]);
  }
  printf("bounding box: x: %d  y: %d  width: %d  height: %d\n", bounding_box.x, bounding_box.y, bounding_box.width, bounding_box.height);
  printf("done\n");
}


static void
gimp_cage_config_class_init (GimpCageConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);

  object_class->set_property       = gimp_cage_config_set_property;
  object_class->get_property       = gimp_cage_config_get_property;
  
  object_class->finalize           = gimp_cage_config_finalize;
}

static void
gimp_cage_config_init (GimpCageConfig *self)
{
  self->cage_vertice_number = 0;
  self->cage_vertices_max = 50; //pre-allocation for 50 vertices for the cage.
  
  self->cage_vertices = g_new(GimpVector2, self->cage_vertices_max);
  self->cage_vertices_d = g_new(GimpVector2, self->cage_vertices_max);
  self->scaling_factor = g_malloc (self->cage_vertices_max * sizeof(gfloat));
}

static void
gimp_cage_config_finalize (GObject *object)
{
  GimpCageConfig  *gcc = GIMP_CAGE_CONFIG (object);
  
  g_free(gcc->cage_vertices);
  g_free(gcc->cage_vertices_d);
  g_free(gcc->scaling_factor);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_cage_config_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  /* GimpCageConfig *gcc = GIMP_CAGE_CONFIG (object); */

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
  /* GimpCageConfig *gcc = GIMP_CAGE_CONFIG (object); */

  switch (property_id)
    {

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_cage_config_add_cage_point (GimpCageConfig  *gcc,
                                 gdouble          x,
                                 gdouble          y)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  
  /* reallocate memory if needed */
  if (gcc->cage_vertice_number >= gcc->cage_vertices_max)
  {
    gcc->cage_vertices_max += N_ITEMS_PER_ALLOC;

    gcc->cage_vertices = g_renew(GimpVector2,
                                gcc->cage_vertices,
                                gcc->cage_vertices_max);
                                
    gcc->cage_vertices_d = g_renew(GimpVector2,
                                gcc->cage_vertices_d,
                                gcc->cage_vertices_max);

    gcc->scaling_factor = g_realloc (gcc->scaling_factor,
                                      gcc->cage_vertices_max * sizeof(gfloat));
  }

  gcc->cage_vertices[gcc->cage_vertice_number].x = x;
  gcc->cage_vertices[gcc->cage_vertice_number].y = y;

  gcc->cage_vertices_d[gcc->cage_vertice_number].x = x;
  gcc->cage_vertices_d[gcc->cage_vertice_number].y = y;
  
  gcc->cage_vertice_number++;
  
  gimp_cage_config_compute_scaling_factor (gcc);
}
                           
void
gimp_cage_config_remove_last_cage_point (GimpCageConfig  *gcc)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  
  if (gcc->cage_vertice_number >= 1)
    gcc->cage_vertice_number--;
  
  gimp_cage_config_compute_scaling_factor (gcc);
}

gint
gimp_cage_config_is_on_handle (GimpCageConfig  *gcc,
                               GimpCageMode     mode,
                               gdouble          x,
                               gdouble          y,
                               gint             handle_size)
{
  gint i;
  gdouble vert_x, vert_y;
  
  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), -1);
  
  if (gcc->cage_vertice_number == 0)
    return -1;
  
  for (i = 0; i < gcc->cage_vertice_number; i++)
  {
    if (mode == GIMP_CAGE_MODE_CAGE_CHANGE)
    {
      vert_x = gcc->cage_vertices[i].x;
      vert_y = gcc->cage_vertices[i].y;
    }
    else
    {
      vert_x = gcc->cage_vertices_d[i].x;
      vert_y = gcc->cage_vertices_d[i].y;
    }
    
    if (x < vert_x + handle_size / 2 && x > vert_x -handle_size / 2 &&
        y < vert_y + handle_size / 2 && y > vert_y -handle_size / 2)
    {
      return i;
    }
  }
  
  return -1;
}
                                              
void
gimp_cage_config_move_cage_point  (GimpCageConfig  *gcc,
                                   GimpCageMode     mode,
                                   gint             point_number,
                                   gdouble          x,
                                   gdouble          y)
{
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->cage_vertice_number);
  g_return_if_fail (point_number >= 0);
  
  if (mode == GIMP_CAGE_MODE_CAGE_CHANGE)
  {
    gcc->cage_vertices[point_number].x = x;
    gcc->cage_vertices[point_number].y = y;
  }
  else
  {
    gcc->cage_vertices_d[point_number].x = x;
    gcc->cage_vertices_d[point_number].y = y;
  }
  
  gimp_cage_config_compute_scaling_factor (gcc);
}
                                                                                                   
GimpVector2
gimp_cage_config_get_edge_normal  (GimpCageConfig  *gcc,
                                   gint             edge_index)
{
  GimpVector2 result;
  
  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), gimp_vector2_new(1,0));
  g_return_val_if_fail (edge_index >= 0, gimp_vector2_new(1,0));
  g_return_val_if_fail (edge_index < gcc->cage_vertice_number, gimp_vector2_new(1,0));
  
  gimp_vector2_sub (&result,
                    &gcc->cage_vertices_d[(edge_index+1) % gcc->cage_vertice_number],
                    &gcc->cage_vertices_d[edge_index]);
  
  return gimp_vector2_normal (&result);
}

GeglRectangle
gimp_cage_config_get_bounding_box       (GimpCageConfig  *gcc)
{
  gint i;
  GeglRectangle bounding_box = {0, };
  
  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), bounding_box);
  g_return_val_if_fail (gcc->cage_vertice_number >= 0, bounding_box);
  
  bounding_box.x = gcc->cage_vertices[0].x;
  bounding_box.y = gcc->cage_vertices[0].y;
  bounding_box.height = 0;
  bounding_box.width = 0;
  
  for (i = 1; i < gcc->cage_vertice_number; i++)
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

void
gimp_cage_config_reverse_cage  (GimpCageConfig *gcc)
{
  gint i;
  GimpVector2 temp;
  
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  
  for (i = 0; i < gcc->cage_vertice_number / 2; i++)
  {
    temp = gcc->cage_vertices[i];
    gcc->cage_vertices[i] = gcc->cage_vertices[gcc->cage_vertice_number - i -1];
    gcc->cage_vertices[gcc->cage_vertice_number - i -1] = temp;
    
    temp = gcc->cage_vertices_d[i];
    gcc->cage_vertices_d[i] = gcc->cage_vertices_d[gcc->cage_vertice_number - i -1];
    gcc->cage_vertices_d[gcc->cage_vertice_number - i -1] = temp;
  }
  
  gimp_cage_config_compute_scaling_factor (gcc);
}

void
gimp_cage_config_reverse_cage_if_needed (GimpCageConfig *gcc)
{
  gint i;
  gdouble sum;
  
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  
  sum = 0.0;
  
  /* this is a bit crappy, but should works most of the case */
  /* we do the sum of the projection of each point to the previous segment, and see the final sign */
  for (i = 0; i < gcc->cage_vertice_number ; i++)
  {
    GimpVector2 P1, P2, P3;
    gdouble z;
    
    P1 = gcc->cage_vertices[i];
    P2 = gcc->cage_vertices[(i+1) % gcc->cage_vertice_number];
    P3 = gcc->cage_vertices[(i+2) % gcc->cage_vertice_number];
    
    z = P1.x * (P2.y - P3.y) + P2.x * (P3.y - P1.y) + P3.x * (P1.y - P2.y);
    
    sum += z;
  }
  
  /* sum > 0 mean a cage defined clockwise, so we reverse it */
  if (sum > 0)
  {
    gimp_cage_config_reverse_cage (gcc);
    printf("reverse the cage !\n");
  }
  else
  {
    printf("Cage OK !\n");
  }
}

static void
gimp_cage_config_compute_scaling_factor (GimpCageConfig *gcc)
{
  gint i;
  gdouble length, length_d;
  GimpVector2 edge;
  
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (gcc));
  
  for (i = 0; i < gcc->cage_vertice_number; i++)
  {
    gimp_vector2_sub ( &edge, &gcc->cage_vertices[i], &gcc->cage_vertices[(i+1) % gcc->cage_vertice_number]);
    length = gimp_vector2_length (&edge);
    
    gimp_vector2_sub ( &edge, &gcc->cage_vertices_d[i], &gcc->cage_vertices_d[(i+1) % gcc->cage_vertice_number]);
    length_d = gimp_vector2_length (&edge);
    
    gcc->scaling_factor[i] = length_d / length;
  }

  print_cage (gcc);
}
