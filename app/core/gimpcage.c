/* GIMP - The GNU Image Manipulation Program
 * 
 * gimpcage.c
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

#include "gimpcage.h"

#include "core-types.h"
#include "libgimpmath/gimpvector.h"
#include <math.h>
#include <gegl-buffer-iterator.h>

#include <stdio.h>

G_DEFINE_TYPE (GimpCage, gimp_cage, G_TYPE_OBJECT)

#define parent_class gimp_cage_parent_class

#define N_ITEMS_PER_ALLOC       10

static void       gimp_cage_finalize                (GObject *object);
static void       gimp_cage_reverse_cage_if_needed  (GimpCage *gc);
static void       gimp_cage_compute_scaling_factor  (GimpCage *gc);
static void       gimp_cage_compute_bounding_box    (GimpCage   *gc);


static void
gimp_cage_class_init (GimpCageClass *klass)
{
  GObjectClass           *object_class         = G_OBJECT_CLASS (klass);
    
  object_class->finalize          = gimp_cage_finalize;
}

static void
gimp_cage_init (GimpCage *self)
{
  self->cage_vertice_number = 0;
  self->cage_vertices_max = 50; //pre-allocation for 50 vertices for the cage.
  self->cage_vertices = g_new(GimpVector2, self->cage_vertices_max);
  self->cage_vertices_d = g_new(GimpVector2, self->cage_vertices_max);
  self->scaling_factor = g_malloc (self->cage_vertices_max * sizeof(gfloat));
  
  self->cage_vertices_coef = NULL;
  self->cage_edges_coef = NULL;
}

static void
gimp_cage_finalize (GObject *object)
{
  GimpCage  *gc = GIMP_CAGE (object);
  
  if (gc->cage_vertices_coef != NULL)
  {
    gegl_buffer_destroy (gc->cage_vertices_coef);
  }
  
  if (gc->cage_edges_coef != NULL)
  {
    gegl_buffer_destroy (gc->cage_edges_coef);
  }
  
  g_free(gc->cage_vertices);
  g_free(gc->scaling_factor);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

                                              
void
gimp_cage_compute_coefficient (GimpCage *gc)
{
  Babl *format;
  GeglBufferIterator *i;
  gint edge, vertice,j;
  
  g_return_if_fail (GIMP_IS_CAGE (gc));
  
  gimp_cage_reverse_cage_if_needed (gc);
  
  gimp_cage_compute_bounding_box (gc);
  

  format = babl_format_n(babl_type("float"), gc->cage_vertice_number);

  
  if(gc->cage_vertices_coef != NULL)
  {
    gegl_buffer_destroy(gc->cage_vertices_coef);
  }
  
  if(gc->cage_edges_coef != NULL)
  {
    gegl_buffer_destroy(gc->cage_edges_coef);
  }
  
  gc->cage_vertices_coef = gegl_buffer_new(&gc->bounding_box, format);
  gc->cage_edges_coef = gegl_buffer_new(&gc->bounding_box, format);
  
  gegl_buffer_clear(gc->cage_vertices_coef, &gc->bounding_box);
  gegl_buffer_clear(gc->cage_edges_coef, &gc->bounding_box);
  
  i = gegl_buffer_iterator_new (gc->cage_vertices_coef, &gc->bounding_box, format, GEGL_BUFFER_READWRITE);
  edge = gegl_buffer_iterator_add (i, gc->cage_edges_coef, &gc->bounding_box, format, GEGL_BUFFER_READWRITE);
  vertice = 0;
  
  //iterate on GeglBuffer
  while (gegl_buffer_iterator_next (i))
  {
    //iterate inside the roi
    gint        n_pixels = i->length;
    gfloat     *vertice_coef = i->data[vertice];
    gfloat     *edge_coef = i->data[edge];
    gint        x = i->roi->x; /* initial x                   */
    gint        y = i->roi->y; /*           and y coordinates */
    

    while(n_pixels--)
    {
      for( j = 0; j < gc->cage_vertice_number; j++)
      {
        GimpVector2 v1,v2,a,b;
        gfloat Q,S,R,BA,SRT,L0,L1,A0,A1,A10,L10;
      
        v1 = gc->cage_vertices[j];
        v2 = gc->cage_vertices[(j+1)%gc->cage_vertice_number];
      
        a.x = v2.x - v1.x;
        a.y = v2.y - v1.y;
        b.x = v1.x - x;
        b.y = v1.y - y;
        Q = a.x * a.x + a.y * a.y;
        S = b.x * b.x + b.y * b.y;
        R = 2.0 * (a.x * b.x + a.y * b.y);
        BA = b.x * a.y - b.y * a.x;
        SRT = sqrt(4.0 * S * Q - R * R);
        L0 = log(S);
        L1 = log(S + Q + R);
        A0 = atan2(R, SRT) / SRT;
        A1 = atan2(2.0 * Q + R, SRT) / SRT;
        A10 = A1 - A0;
        L10 = L1 - L0;
        
        edge_coef[j] = 1.0 / (4.0 * M_PI) * ((4.0*S-R*R/Q) * A10 + R / (2.0 * Q) * L10 + L1 - 2.0);
        
        vertice_coef[j] += BA / (2.0 * M_PI) * (L10 /(2.0*Q) - A10 * (2.0 + R / Q));
        vertice_coef[(j+1)%gc->cage_vertice_number] -= BA / (2.0 * M_PI) * (L10 / (2.0 * Q) - A10 * R / Q);
      }
      
      vertice_coef += gc->cage_vertice_number;
      edge_coef += gc->cage_vertice_number;
      
      /* update x and y coordinates */
      x++;
      if (x >= (i->roi->x + i->roi->width))
      {
        x = i->roi->x;
        y++;
      }
      
    }
  }
}

void
gimp_cage_compute_scaling_factor (GimpCage *gc)
{
  gint i;
  gdouble length, length_d;
  GimpVector2 edge;
  
  g_return_if_fail (GIMP_IS_CAGE (gc));
  
  for (i = 0; i < gc->cage_vertice_number; i++)
  {
    gimp_vector2_sub ( &edge, &gc->cage_vertices[i], &gc->cage_vertices[(i+1) % gc->cage_vertice_number]);
    length = gimp_vector2_length (&edge);
    
    gimp_vector2_sub ( &edge, &gc->cage_vertices_d[i], &gc->cage_vertices_d[(i+1) % gc->cage_vertice_number]);
    length_d = gimp_vector2_length (&edge);
    
    gc->scaling_factor[i] = length_d / length;
  }
}

void
gimp_cage_add_cage_point  (GimpCage    *gc,
                           gdouble      x,
                           gdouble      y)
{
  g_return_if_fail (GIMP_IS_CAGE (gc));
  
  /* reallocate memory if needed */
  if (gc->cage_vertice_number >= gc->cage_vertices_max)
  {
    gc->cage_vertices_max += N_ITEMS_PER_ALLOC;

    gc->cage_vertices = g_renew(GimpVector2,
                                gc->cage_vertices,
                                gc->cage_vertices_max);
                                
    gc->cage_vertices_d = g_renew(GimpVector2,
                                gc->cage_vertices_d,
                                gc->cage_vertices_max);
                                
    gc->scaling_factor = g_realloc (gc->scaling_factor,
                                    gc->cage_vertices_max * sizeof(gfloat));
  }

  gc->cage_vertices[gc->cage_vertice_number].x = x;
  gc->cage_vertices[gc->cage_vertice_number].y = y;

  gc->cage_vertices_d[gc->cage_vertice_number].x = x;
  gc->cage_vertices_d[gc->cage_vertice_number].y = y;
  
  gc->cage_vertice_number++;
  
  gimp_cage_compute_scaling_factor (gc);
}

void
gimp_cage_remove_last_cage_point (GimpCage    *gc)
{
  g_return_if_fail (GIMP_IS_CAGE (gc));
  
  if (gc->cage_vertice_number >= 1)
    gc->cage_vertice_number--;
}



gint
gimp_cage_is_on_handle  (GimpCage    *gc,
                         gdouble      x,
                         gdouble      y,
                         gint         handle_size)
{
  gint i;
  gdouble vert_x, vert_y;
  
  g_return_val_if_fail (GIMP_IS_CAGE (gc), -1);
  
  if (gc->cage_vertice_number == 0)
    return -1;
  
  for (i = 0; i < gc->cage_vertice_number; i++)
  {
    vert_x = gc->cage_vertices[i].x;
    vert_y = gc->cage_vertices[i].y;
    
    if (x < vert_x + handle_size / 2 && x > vert_x -handle_size / 2 &&
        y < vert_y + handle_size / 2 && y > vert_y -handle_size / 2)
    {
      return i;
    }
  }
  
  return -1;
}

gint
gimp_cage_is_on_handle_d  (GimpCage    *gc,
                           gdouble      x,
                           gdouble      y,
                           gint         handle_size)
{
  gint i;
  gdouble vert_x, vert_y;
  
  g_return_val_if_fail (GIMP_IS_CAGE (gc), -1);
  
  if (gc->cage_vertice_number == 0)
    return -1;
  
  for (i = 0; i < gc->cage_vertice_number; i++)
  {
    vert_x = gc->cage_vertices_d[i].x;
    vert_y = gc->cage_vertices_d[i].y;
    
    if (x < vert_x + handle_size / 2 && x > vert_x -handle_size / 2 &&
        y < vert_y + handle_size / 2 && y > vert_y -handle_size / 2)
    {
      return i;
    }
  }
  
  return -1;
}

void
gimp_cage_move_cage_point  (GimpCage    *gc,
                            gint         point_number,
                            gdouble      x,
                            gdouble      y)
{
  g_return_if_fail (GIMP_IS_CAGE (gc));
  g_return_if_fail (point_number < gc->cage_vertice_number);
  
  gc->cage_vertices[point_number].x = x;
  gc->cage_vertices[point_number].y = y;
}

void
gimp_cage_move_cage_point_d  (GimpCage    *gc,
                              gint         point_number,
                              gdouble      x,
                              gdouble      y)
{
  g_return_if_fail (GIMP_IS_CAGE (gc));
  g_return_if_fail (point_number < gc->cage_vertice_number);
  
  gc->cage_vertices_d[point_number].x = x;
  gc->cage_vertices_d[point_number].y = y;
  gimp_cage_compute_scaling_factor (gc);
}

/*
 * The cage need to be defined in a counter-clockwise way, so depending on how the user setup it, we may need to reverse the order of the point
 */
static void
gimp_cage_reverse_cage  (GimpCage *gc)
{
  gint i;
  GimpVector2 temp;
  
  g_return_if_fail (GIMP_IS_CAGE (gc));
  
  for (i = 0; i < gc->cage_vertice_number / 2; i++)
  {
    temp = gc->cage_vertices[i];
    gc->cage_vertices[i] = gc->cage_vertices[gc->cage_vertice_number - i -1];
    gc->cage_vertices[gc->cage_vertice_number - i -1] = temp;
    
    temp = gc->cage_vertices_d[i];
    gc->cage_vertices_d[i] = gc->cage_vertices_d[gc->cage_vertice_number - i -1];
    gc->cage_vertices_d[gc->cage_vertice_number - i -1] = temp;
  }
  
  gimp_cage_compute_scaling_factor (gc);
}

static void
gimp_cage_reverse_cage_if_needed (GimpCage *gc)
{
  gint i;
  gdouble sum;
  
  g_return_if_fail (GIMP_IS_CAGE (gc));
  
  sum = 0.0;
  
  /* we sum all the angles in the cage */
  for (i = 0; i < gc->cage_vertice_number ; i++)
  {
    GimpVector2 P1, P2, P3;
    gdouble z;
    
    P1 = gc->cage_vertices[i];
    P2 = gc->cage_vertices[(i+1) % gc->cage_vertice_number];
    P3 = gc->cage_vertices[(i+2) % gc->cage_vertice_number];
    
    z = P1.x * (P2.y - P3.y) + P2.x * (P3.y - P1.y) + P3.x * (P1.y - P2.y);
    
    sum += z;
  }
  
  /* sum > 0 mean a cage defined clockwise, so we reverse it */
  if (sum > 0)
  {
    gimp_cage_reverse_cage (gc);
  }
}

GimpVector2
gimp_cage_get_edge_normal  (GimpCage *gc,
                            gint      edge_index)
{
  GimpVector2 result;
  
  g_return_val_if_fail (GIMP_IS_CAGE (gc), gimp_vector2_new(1,0));
  g_return_val_if_fail (edge_index >= 0, gimp_vector2_new(1,0));
  g_return_val_if_fail (edge_index < gc->cage_vertice_number, gimp_vector2_new(1,0));
  
  gimp_vector2_sub (&result,
                    &gc->cage_vertices_d[(edge_index+1) % gc->cage_vertice_number],
                    &gc->cage_vertices_d[edge_index]);
  
  return gimp_vector2_normal (&result);
}

static void
gimp_cage_compute_bounding_box  (GimpCage   *gc)
{
  gint i;
  
  g_return_if_fail (GIMP_IS_CAGE (gc));
  g_return_if_fail (gc->cage_vertice_number >= 0);
  
  gc->bounding_box.x = gc->cage_vertices[0].x;
  gc->bounding_box.y = gc->cage_vertices[0].y;
  gc->bounding_box.height = 1;
  gc->bounding_box.width = 1;
  
  for (i = 1; i < gc->cage_vertice_number; i++)
  {
    gdouble x,y;
    
    x = gc->cage_vertices[i].x;
    y = gc->cage_vertices[i].y;
    
    if (x < gc->bounding_box.x)
    {
      gc->bounding_box.x = x;
    }
    
    if (y < gc->bounding_box.y)
    {
      gc->bounding_box.y = y;
    }
    
    if (x > gc->bounding_box.x + gc->bounding_box.width)
    {
      gc->bounding_box.width = x - gc->bounding_box.x;
    }
    
    if (y > gc->bounding_box.y + gc->bounding_box.height)
    {
      gc->bounding_box.height = y - gc->bounding_box.y;
    }
  }
}