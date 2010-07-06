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


G_DEFINE_TYPE (GimpCage, gimp_cage, G_TYPE_OBJECT)

static void       gimp_cage_compute_coefficient (GimpCage *gc);

static void
gimp_cage_class_init (GimpCageClass *klass)
{

}

static void
gimp_cage_init (GimpCage *self)
{
  //Test Data
  self->cage_vertice_number = 4;
  self->cage_vertices = g_new(GimpVector2, self->cage_vertice_number);
  self->cage_vertices_max = self->cage_vertice_number;

  self->cage_vertices[0].x = 20;
  self->cage_vertices[0].y = 20;
  self->cage_vertices[1].x = 20;
  self->cage_vertices[1].y = 100;
  self->cage_vertices[2].x = 100;
  self->cage_vertices[2].y = 100;
  self->cage_vertices[3].x = 100;
  self->cage_vertices[3].y = 20;
  
  self->extent.x = 20;
  self->extent.y = 20;
  self->extent.height = 80;
  self->extent.width = 80;
  
  self->cage_vertices_coef = NULL;
  self->cage_edges_coef = NULL;
  
  gimp_cage_compute_coefficient (self);
}

static void
gimp_cage_compute_coefficient (GimpCage *gc)
{
  Babl *format;
  GeglBufferIterator *i;
  gint edge, vertice,j;
  
  g_return_if_fail (GIMP_IS_CAGE (gc));
  
  format = babl_format_n(babl_type("float"), gc->cage_vertice_number);

  
  if(gc->cage_vertices_coef != NULL)
  {
    gegl_buffer_destroy(gc->cage_vertices_coef);
  }
  
  if(gc->cage_edges_coef != NULL)
  {
    gegl_buffer_destroy(gc->cage_edges_coef);
  }
  
  gc->cage_vertices_coef = gegl_buffer_new(&gc->extent, format);
  gc->cage_edges_coef = gegl_buffer_new(&gc->extent, format);
  
  gegl_buffer_clear(gc->cage_vertices_coef, &gc->extent);
  gegl_buffer_clear(gc->cage_edges_coef, &gc->extent);
  
  i = gegl_buffer_iterator_new (gc->cage_vertices_coef, &gc->extent, format, GEGL_BUFFER_WRITE);
  edge = gegl_buffer_iterator_add (i, gc->cage_edges_coef, &gc->extent, format, GEGL_BUFFER_WRITE);
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
        
        vertice_coef += gc->cage_vertice_number;
        edge_coef += gc->cage_vertice_number;
      }
      
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
